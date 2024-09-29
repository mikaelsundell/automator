// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "queue.h"
#include "process.h"
#include "mac.h"

#include <QObject>
#include <QPointer>
#include <QThreadPool>
#include <QtConcurrent>
#include <QCoreApplication>
#include <QDebug>

#define THREAD_FUNC_SAFE() static QMutex mutex; QMutexLocker locker(&mutex);
#define THREAD_OBJECT_SAFE(obj) static QMutex obj##_mutex; QMutexLocker locker(&obj##_mutex);

QScopedPointer<Queue, Queue::Deleter> Queue::pi;

class QueuePrivate : public QObject
{
    Q_OBJECT
    public:
        QueuePrivate();
        void init();
        void update();
        QUuid submit(QSharedPointer<Job> job);
        void start(const QUuid& uuid);
        void stop(const QUuid& uuid);
        void restart(const QUuid& uuid);
        void remove(const QUuid& uuid);
        void processJob(QSharedPointer<Job> job);
        QSharedPointer<Job> findNextJob();
        void processNextJobs();
        void processRemovedJobs();
        void processDependentJobs(const QUuid& dependsonUuid);
        void failDependentJobs(const QUuid& dependsonId);
        void failCompletedJobs(const QUuid& uuid, const QUuid& dependsonId);

    public Q_SLOTS:
        void statusChanged(const QUuid& uuid, Job::Status status);
    
    Q_SIGNALS:
        void notifyStatusChanged(const QUuid& uuid, Job::Status status);
    
    public:
        int threads;
        QMutex mutex;
        QThread thread;
        QThreadPool threadPool;
        QMap<QUuid, QSharedPointer<Job>> allJobs;
        QList<QSharedPointer<Job>> waitingJobs;
        QSet<QUuid> completedJobs;
        QMap<QUuid, QList<QSharedPointer<Job>>> dependentJobs;
        QMap<QUuid, QSharedPointer<Job>> removedJobs;
        QPointer<Queue> queue;
};

QueuePrivate::QueuePrivate()
: threads(1)
{
    threadPool.setMaxThreadCount(threads);
    threadPool.setExpiryTimeout(-1);
}

void
QueuePrivate::init()
{
    // treads
    QThreadPool::globalInstance()->setThreadPriority(QThread::LowPriority); // scheduled less often than ui thread
    // connect
    connect(this, &QueuePrivate::notifyStatusChanged, this, &QueuePrivate::statusChanged, Qt::QueuedConnection);
    update();
}

void
QueuePrivate::update()
{
    threadPool.setMaxThreadCount(threads);
}

QUuid
QueuePrivate::submit(QSharedPointer<Job> job)
{
    {
        QMutexLocker locker(&mutex);
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
            waitingJobs.append(job);
        } else {
            dependentJobs[job->dependson()].append(job);
        }
    }
    processNextJobs();
    processRemovedJobs();
    queue->jobSubmitted(job);
    return job->uuid();
}

void
QueuePrivate::start(const QUuid& uuid)
{
    bool start = false;
    {
        QMutexLocker locker(&mutex);
        QSharedPointer<Job> job = allJobs[uuid];
        if (job->status() == Job::Stopped) {
            job->setStatus(Job::Waiting);
            waitingJobs.append(job);
            QString log = QString("Uuid:\n"
                                  "%1\n\n"
                                  "Command:\n"
                                  "%2 %3\n")
                                  .arg(job->uuid().toString())
                                  .arg(job->command())
                                  .arg(job->arguments().join(' '));
            job->setLog(log);
            start = true;
        }
    }
    if (start) {
        processNextJobs();
    }
}

void
QueuePrivate::stop(const QUuid& uuid)
{
    {
        QMutexLocker locker(&mutex);
        QSharedPointer<Job> job = allJobs[uuid];
        if (job->status() == Job::Running) {
            job->setStatus(Job::Stopped);
            int pid = job->pid();
            if (pid > 0) {
                Process::kill(job->pid());
            }
            QString log = QString("Uuid:\n"
                                  "%1\n\n"
                                  "Command:\n"
                                  "%2 %3\n")
                                  .arg(job->uuid().toString())
                                  .arg(job->command())
                                  .arg(job->arguments().join(' '));
            job->setLog(log);
        }
    }
    processNextJobs();
}

void
QueuePrivate::restart(const QUuid& uuid)
{
    {
        QMutexLocker locker(&mutex);
        std::function<void(const QUuid&)> restartJob = [&](const QUuid& jobUuid) {
            QSharedPointer<Job> job = allJobs[jobUuid];
            if (job->status() != Job::Running) {
                job->setStatus(Job::Waiting);
                if (job->dependson().isNull()) {
                    waitingJobs.append(job);
                } else {
                    if (!dependentJobs[job->dependson()].contains(job)) {
                        dependentJobs[job->dependson()].append(job);
                    }
                }
                QString log = QString("Uuid:\n"
                                      "%1\n\n"
                                      "Command:\n"
                                      "%2 %3\n")
                                      .arg(job->uuid().toString())
                                      .arg(job->command())
                                      .arg(job->arguments().join(' '));
                job->setLog(log);
                for (QSharedPointer<Job>& dependentJob : allJobs) {
                    if (dependentJob->dependson() == jobUuid) {
                        restartJob(dependentJob->uuid());
                    }
                }
                
            }
        };
        restartJob(uuid);
    }
    processNextJobs();
}

void
QueuePrivate::remove(const QUuid& uuid)
{
    QList<QUuid> dependentUuids;
    {
        QMutexLocker locker(&mutex);
        if (allJobs.contains(uuid)) {
            QSharedPointer<Job> job = allJobs.take(uuid);
            removedJobs.insert(uuid, job); // mark as removed for threads
            if (job->status() == Job::Running) {
                int pid = job->pid();
                if (pid > 0) {
                    Process::kill(job->pid());
                }
            }
            for (auto it = allJobs.begin(); it != allJobs.end(); ++it) {
                QSharedPointer<Job> job = it.value();
                if (job->dependson() == uuid) {
                    dependentUuids.append(job->uuid());
                }
            }
            dependentJobs.remove(uuid); // prevent from trying to fail dependent deleted jobs
            waitingJobs.removeAll(job);
            completedJobs.remove(uuid);
            queue->jobProcessed(uuid); // mark as processed, it's not removed
        }
    }
    for (const QUuid& dependentUuid : dependentUuids) {
        remove(dependentUuid);
    }
    queue->jobRemoved(uuid);
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
        job->setStatus(Job::Running);
        bool exists = false;
        QString output = job->output();
        QFileInfo dirInfo(output);
        if (!dirInfo.exists()) {
            QDir dir;
            if (!dir.mkdir(output)) {
                log += QString("\nStatus:\n"
                               "Could not create directory: %1\n")
                               .arg(output);
                
                job->setStatus(Job::Failed);
            } else {
                exists = true;
            }
        } else if (!dirInfo.isDir()) {
            log += QString("\nStatus:\n"
                               "Output exists but is not a directory: %1\n")
                               .arg(output);
            job->setStatus(Job::Failed);
        } else {
            exists = true;
        }
        if (exists)
        {
            bool failed = false;
            bool stopped = false;
            QScopedPointer<Process> process(new Process());
            QString standardoutput;
            QString standarderror;
            if (process->exists(command)) {
                process->run(command, job->arguments(), job->startin());
                int pid = process->pid();
                job->setPid(pid);
                log += QString("\nProcess id:\n%1\n").arg(pid);
                job->setLog(log);
                if (process->wait()) {
                    job->setStatus(Job::Completed);
                    log += QString("\nStatus:\n%1\n").arg("Command completed");
                } else {
                    if (job->status() == Job::Stopped) {
                        stopped = true;
                    } else {
                        failed = true;
                    }
                }
                standardoutput = process->standardOutput();
                standarderror = process->standardError();
            } else {
                standarderror = "Command does not exists, make sure command can be "
                                "found in system or application search paths";
                failed = true;
            }
            if (failed) {
                log += QString("\nStatus:\n%1\n").arg("Command failed");
                log += QString("\nExit code:\n%1\n").arg(process->exitCode());
                switch(process->exitStatus())
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
                
            }
            if (stopped) {
                log += QString("\nStatus:\n%1\n").arg("Command stopped");
            }
            if (!standardoutput.isEmpty()) {
                log += QString("\nCommand output:\n%1").arg(standardoutput);
            }
            if (!standarderror.isEmpty()) {
                log += QString("\nCommand error:\n%1").arg(standarderror);
            }
        }
    }
    job->setLog(log);
    if (job->status() == Job::Failed) {
        if (!job->dependson().isNull()) {
            failCompletedJobs(job->uuid(), job->dependson());
        }
    }
    if (job->status() != Job::Stopped) {
        queue->jobProcessed(job->uuid());
    }
}

QSharedPointer<Job>
QueuePrivate::findNextJob()
{
    int selectedIndex = 0;
    QSharedPointer<Job> selectedJob = waitingJobs.first();
    for (int i = 1; i < waitingJobs.size(); ++i) {
        QSharedPointer<Job> job = waitingJobs[i];
        if (job->status() == Job::Waiting) {
            if (job->priority() > selectedJob->priority()) {
                selectedJob = job;
                selectedIndex = i;
            }
            else if (job->priority() == selectedJob->priority() && job->created() < selectedJob->created()) {
                selectedJob = job;
                selectedIndex = i;
            }
        }
    }
    return waitingJobs.takeAt(selectedIndex);
}

void
QueuePrivate::processNextJobs()
{
    QMutexLocker locker(&mutex);
    int free = threadPool.maxThreadCount() - threadPool.activeThreadCount();
    int jobsprocess = qMin(waitingJobs.size(), free);
    if (jobsprocess == 0) {
        return;
    }
    QList<QSharedPointer<Job>> jobsrun;
    for (int i = 0; i < jobsprocess; ++i) {
        if (!waitingJobs.isEmpty()) {
            jobsrun.append(findNextJob());
        }
    }
    for (QSharedPointer<Job>& job : jobsrun) {
        QFuture<void> future = QtConcurrent::run(&threadPool, [this, job]() {
            processJob(job);
        });
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
        connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher, job]() {
            watcher->deleteLater();
            statusChanged(job->uuid(), job->status());
        });
        watcher->setFuture(future);
    }
}

void
QueuePrivate::processRemovedJobs()
{
    removedJobs.clear(); // safe to clear at submit, all event are processed
}

void
QueuePrivate::processDependentJobs(const QUuid& dependsonId)
{
    if (dependentJobs.contains(dependsonId)) {
        for (QSharedPointer<Job> job : dependentJobs[dependsonId]) {
            waitingJobs.append(job);
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
            job->setStatus(Job::Failed);
            queue->jobProcessed(job->uuid());
            notifyStatusChanged(job->uuid(), job->status());
            failDependentJobs(job->uuid());
        }
        dependentJobs.remove(dependsonId);
    }
}

void
QueuePrivate::failCompletedJobs(const QUuid& uuid, const QUuid& dependsonId)
{
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
    {
        QMutexLocker locker(&mutex);
        if (!removedJobs.contains(uuid)) {
            if (status == Job::Completed) {
                completedJobs.insert(uuid); // Mark the job as completed
                processDependentJobs(uuid);
            } else if (status == Job::Failed) {
                failDependentJobs(uuid);
            }
        }
    }
    processNextJobs();
}

#include "queue.moc"

Queue::Queue()
: p(new QueuePrivate())
{
    p->moveToThread(&p->thread);
    p->queue = this;
    p->init();
    p->thread.start();
}

Queue::~Queue()
{
    p->threadPool.waitForDone();
    p->thread.quit();
    p->thread.wait();
}

Queue*
Queue::instance()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (!pi) {
        pi.reset(new Queue());
    }
    return pi.data();
}

QUuid
Queue::submit(QSharedPointer<Job> job)
{
    return p->submit(job);
}

void
Queue::start(const QUuid& uuid)
{
    p->start(uuid);
}

void
Queue::stop(const QUuid& uuid)
{
    p->stop(uuid);
}

void
Queue::restart(const QUuid& uuid)
{
    p->restart(uuid);
}

void
Queue::remove(const QUuid& uuid)
{
    p->remove(uuid);
}

int
Queue::threads() const
{
    return p->threads;
}

void
Queue::setThreads(int threads)
{
    p->threads = threads;
    p->update();
    p->processNextJobs();
}
