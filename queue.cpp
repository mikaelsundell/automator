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
        QUuid submit(QSharedPointer<Job> job);
        void processNextJob();
        void processDependentJobs(const QUuid& dependsonUuid);
        void failDependentJobs(const QUuid& dependsonId);
    
    public Q_SLOTS:
        void statusChanged(const QUuid& uuid, Job::Status status);
    
    Q_SIGNALS:
        void notifyStatusChanged(const QUuid& uuid, Job::Status status);
    
    public:
        QQueue<QSharedPointer<Job>> pendingJobs;
        QSet<QUuid> completedJobs;
        QMap<QUuid, QList<QSharedPointer<Job>>> dependentJobs;
        QPointer<Queue> queue;
};

QueuePrivate::QueuePrivate()
{
}

void
QueuePrivate::init()
{
    int maxThreads = 2;
    // connect
    connect(this, &QueuePrivate::notifyStatusChanged, this, &QueuePrivate::statusChanged);
    // threads
    QThreadPool::globalInstance()->setMaxThreadCount(maxThreads);
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
    if (pendingJobs.isEmpty()/* || QThreadPool::globalInstance()->activeThreadCount() >= QThreadPool::globalInstance()->maxThreadCount()*/) {

        return;
    }

    QSharedPointer<Job> job = pendingJobs.dequeue();
    job->setStatus(Job::Running);
    QtConcurrent::run([this, job]() {

        qDebug()  << "Running on thread: " << QThread::currentThread();
        
        Process process;
        process.run(job->command(), job->arguments().split(" "));
        
        
        job->setLog(
            QString("Command:\n"
                    "%1 %2\n"
                    "\n"
                    "Standart-out:\n"
                    "%3\n"
                    "Standard error:\n"
                    "%4")
                    .arg(job->command())
                    .arg(job->arguments())
                    .arg(process.standardOutput())
                    .arg(process.standardError())
        );
        
        // On success
        job->setStatus(Job::Completed);
        
        // On failure
        // job->status = Job::Failed;
        
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
    qDebug() << "statusChanged: " << uuid;
    
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

