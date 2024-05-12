// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "queue.h"
#include "process.h"

#include <QObject>
#include <QPointer>
#include <QThreadPool>
#include <QtConcurrent>

#include <QDebug>

#define THREAD_SAFE() static QMutex mutex; QMutexLocker locker(&mutex);

class QueuePrivate : public QObject
{
    Q_OBJECT
    public:
        QueuePrivate();
        void init();
        void update();
        QUuid submit(QSharedPointer<Job> job);
        void processJob(QSharedPointer<Job> job);
        QSharedPointer<Job> findNextJob();
        void processNextJobs();
        void processDependentJobs(const QUuid& dependsonUuid);
        void failDependentJobs(const QUuid& dependsonId);
        void failCompletedJobs(const QUuid& uuid, const QUuid& dependsonId);

    public Q_SLOTS:
        void statusChanged(const QUuid& uuid, Job::Status status);
    
    Q_SIGNALS:
        void notifyStatusChanged(const QUuid& uuid, Job::Status status);
    
    public:
        int threads;
        QMap<QUuid, QSharedPointer<Job>> allJobs;
        QList<QSharedPointer<Job>> pendingJobs;
        QSet<QUuid> completedJobs;
        QMap<QUuid, QList<QSharedPointer<Job>>> dependentJobs;
        QPointer<Queue> queue;
};

QueuePrivate::QueuePrivate()
: threads(1)
{
}

void
QueuePrivate::init()
{
    // connect
    connect(this, &QueuePrivate::notifyStatusChanged, this, &QueuePrivate::statusChanged);
    update();
}

void
QueuePrivate::update()
{
    QThreadPool::globalInstance()->setMaxThreadCount(threads);
}

QUuid
QueuePrivate::submit(QSharedPointer<Job> job) {
    QString log = QString("Uuid:\n"
                          "%1\n\n"
                          "Command:\n"
                          "%2 %3\n")
                          .arg(job->uuid().toString())
                          .arg(job->command())
                          .arg(job->arguments().join(' '));
    job->setLog(log);
    allJobs.insert(job->uuid(), job);
    if (job->dependson().isNull() || completedJobs.contains(job->dependson())) {
        pendingJobs.append(job);
    } else {
        dependentJobs[job->dependson()].append(job);
    }
    processNextJobs();
    return job->uuid();
}

void
QueuePrivate::processJob(QSharedPointer<Job> job)
{
    QString log = job->log();
    QFileInfo commandInfo(job->command());
    if (commandInfo.isAbsolute() && !commandInfo.exists()) {
        log += QString("\nCommand error:\nCommand path could not be found: %1\n").arg(job->command());
        job->setStatus(Job::Failed);
    } else {
        QString command = job->command();
        if (!commandInfo.isAbsolute()) {
            QSettings settings(MACOSX_BUNDLE_GUI_IDENTIFIER, "Automator");
            QStringList searchpaths = settings.value("searchpaths", QStringList()).toStringList();
            for(QString searchpath : searchpaths) {
                QString filepath = QDir::cleanPath(QDir(searchpath).filePath(command));
                if (QFile::exists(filepath)) {
                    command = filepath;
                    break;
                }
            }
        }
        Process process;
        job->setStatus(Job::Running);
        QString standardOutput;
        QString standardError;
        bool failed = false;
        if (process.exists(command)) {
            if (process.run(command, job->arguments(), job->startin())) {
                job->setStatus(Job::Completed);
                log += QString("\nStatus:\n%1\n").arg("Command completed");
            } else {
                failed = true;
            }
            standardOutput = process.standardOutput();
            standardError = process.standardError();
        } else {
            standardError = "Command does not exists, make sure command can be "
                            "found in system or application search paths";
            failed = true;
        }
        if (failed) {
            log += QString("\nStatus:\n%1\n").arg("Command failed");
            log += QString("\nExit code:\n%1\n").arg(process.exitCode());
            switch(process.exitStatus())
            {
                case Process::Normal: {
                    log += QString("\nExit status:\n%1\n").arg("Normal");
                }
                break;
                case Process::Crash: {
                    log += QString("\nExit status:\n%1\n").arg("Crash");
                }
                break;
            }
            job->setStatus(Job::Failed);
            if (!job->dependson().isNull()) {
                failCompletedJobs(job->uuid(), job->dependson());
            }
        }
        if (!standardOutput.isEmpty()) {
            log += QString("\nCommand output:\n%1").arg(standardOutput);
        }
        if (!standardError.isEmpty()) {
            log += QString("\nCommand error:\n%1").arg(standardError);
        }
    }
    job->setLog(log);
    queue->jobProcessed(job->uuid());
    notifyStatusChanged(job->uuid(), job->status());
}

QSharedPointer<Job>
QueuePrivate::findNextJob() {
    THREAD_SAFE();
    int createdindex = 0;
    QDateTime created = pendingJobs.first()->created();
    for (int i = 1; i < pendingJobs.size(); ++i) {
        if (pendingJobs[i]->created() < created) {
            created = pendingJobs[i]->created();
            createdindex = i;
        }
    }
    return pendingJobs.takeAt(createdindex);
}

void
QueuePrivate::processNextJobs()
{
    int free =
        QThreadPool::globalInstance()->maxThreadCount() -
        QThreadPool::globalInstance()->activeThreadCount();

    int jobsprocess = qMin(pendingJobs.size(), free);
    if (jobsprocess == 0) {
        return;
    }
    QList<QSharedPointer<Job>> jobsrun;
    for (int i = 0; i < jobsprocess; ++i) {
        if (!pendingJobs.isEmpty()) {
            jobsrun.append(findNextJob());
        }
    }
    for (QSharedPointer<Job>& job : jobsrun) {
        QFuture<void> future = QtConcurrent::run([this, job]() {
            processJob(job);
        });
    }
}

void
QueuePrivate::processDependentJobs(const QUuid& dependsonId)
{
    if (dependentJobs.contains(dependsonId)) {
        for (QSharedPointer<Job> job : dependentJobs[dependsonId]) {
            pendingJobs.append(job);
        }
        dependentJobs.remove(dependsonId);
    }
}

void
QueuePrivate::failDependentJobs(const QUuid& dependsonId) {
    if (dependentJobs.contains(dependsonId)) {
        for (QSharedPointer<Job> job : dependentJobs[dependsonId]) {
            QString log = QString("Uuid:\n"
                                  "%1\n\n"
                                  "Command:\n"
                                  "%2 %3\n\n"
                                  "Status:\n"
                                  "Command cancelled, dependent job failed: %4")
                                  .arg(job->uuid().toString())
                                  .arg(job->command())
                                  .arg(job->arguments().join(' '))
                                  .arg(dependsonId.toString());
            job->setLog(log);
            job->setStatus(Job::Cancelled);
            queue->jobProcessed(job->uuid());
            notifyStatusChanged(job->uuid(), job->status());
            failDependentJobs(job->uuid());
        }
        dependentJobs.remove(dependsonId);
    }
}

void
QueuePrivate::failCompletedJobs(const QUuid& uuid, const QUuid& dependsonId) {
    if (allJobs.contains(dependsonId)) {
        QSharedPointer<Job> job = allJobs[dependsonId];
        QString log = job->log();
        log += QString("\nDependent error:\n%1").arg("Dependent job failed: %1").arg(uuid.toString());
        job->setLog(log);
        job->setStatus(Job::Dependency);
        if (!job->dependson().isNull()) {
            failCompletedJobs(dependsonId, job->dependson());
        }
    }
}

void
QueuePrivate::statusChanged(const QUuid& uuid, Job::Status status)
{
    if (status == Job::Completed) {
        completedJobs.insert(uuid); // Mark the job as completed
        processDependentJobs(uuid);
    } else if (status == Job::Failed) {
        failDependentJobs(uuid);
    }
    processNextJobs();
}

#include "queue.moc"

Queue::Queue()
: p(new QueuePrivate())
{
    p->queue = this;
    p->init();
}

Queue::~Queue()
{
}

QUuid
Queue::submit(QSharedPointer<Job> job)
{
    return p->submit(job);
}

void
Queue::setThreads(int threads)
{
    p->threads = threads;
    p->update();
}

