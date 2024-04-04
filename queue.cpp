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

class QueuePrivate : public QObject
{
    Q_OBJECT
    public:
        QueuePrivate();
        void init();
        void update();
        QUuid submit(QSharedPointer<Job> job);
        void processNextJob();
        void processDependentJobs(const QUuid& dependsonUuid);
        void failDependentJobs(const QUuid& dependsonId);

    public Q_SLOTS:
        void statusChanged(const QUuid& uuid, Job::Status status);
    
    Q_SIGNALS:
        void notifyStatusChanged(const QUuid& uuid, Job::Status status);
    
    public:
        int threads;
        QQueue<QSharedPointer<Job>> pendingJobs;
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

QUuid QueuePrivate::submit(QSharedPointer<Job> job) {
    job->setUuid(QUuid::createUuid());
    if (job->dependson().isNull() || completedJobs.contains(job->dependson())) {
        pendingJobs.enqueue(job);
    } else {
        dependentJobs[job->dependson()].append(job);
    }
    processNextJob();
    return job->uuid();
}
void
QueuePrivate::processNextJob()
{
    if (pendingJobs.isEmpty()) {
        return;
    }
    QSharedPointer<Job> job = pendingJobs.dequeue();
    QtConcurrent::run([this, job]() {

        QString log = QString("Uuid:\n"
                              "%1\n\n"
                              "Command:\n"
                              "%2 %3\n")
                              .arg(job->uuid().toString())
                              .arg(job->command())
                              .arg(job->arguments().join(' '));
        
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
            if (process.run(command, job->arguments(), job->startin())) {
                job->setStatus(Job::Completed);
            } else {
                job->setStatus(Job::Failed);
            }
            
            const QString standardOutput = process.standardOutput();
            if (!standardOutput.isEmpty()) {
                log += QString("\nCommand output:\n%1\n").arg(standardOutput);
            }

            const QString standardError = process.standardError();
            if (!standardError.isEmpty()) {
                log += QString("\nCommand error:\n%1\n").arg(standardError);
            }
        }
        
        job->setLog(log);
        queue->jobProcessed(job->uuid());
        notifyStatusChanged(job->uuid(), job->status());
    });
}

void
QueuePrivate::processDependentJobs(const QUuid& dependsonId)
{
    if (dependentJobs.contains(dependsonId)) {
        for (QSharedPointer<Job> job : dependentJobs[dependsonId]) {
            pendingJobs.enqueue(job);
        }
        dependentJobs.remove(dependsonId);
    }
}

void
QueuePrivate::failDependentJobs(const QUuid& dependsonId) {
    if (dependentJobs.contains(dependsonId)) {
        for (QSharedPointer<Job> job : dependentJobs[dependsonId]) {
            job->setStatus(Job::Failed);
            notifyStatusChanged(job->uuid(), job->status());
        }
        dependentJobs.remove(dependsonId);
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
    processNextJob();
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

