// Copyright 2022-present Contributors to the jobman project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/jobman

#pragma once

#include "job.h"

#include <QObject>
#include <QScopedPointer>

class QueuePrivate;
class Queue : public QObject
{
    Q_OBJECT
    public:
        static Queue* instance();
        QUuid submit(QSharedPointer<Job> job);
        void submit(QList<QSharedPointer<Job>> jobs);
        void start(const QUuid& uuid);
        void stop(const QUuid& uuid);
        void restart(const QUuid& uuid);
        void remove(const QUuid& uuid);
        int threads() const;
        void setThreads(int threads);
    
    Q_SIGNALS:
        void jobSubmitted(QSharedPointer<Job> job);
        void jobProcessed(const QUuid& uuid);
        void jobRemoved(const QUuid& uuid);

    private:
        Queue();
        ~Queue();
        Queue(const Queue&) = delete;
        Queue& operator=(const Queue&) = delete;
        class Deleter {
        public:
            static void cleanup(Queue* pointer) {
                delete pointer;
            }
        };
        static QScopedPointer<Queue, Deleter> pi;
        QScopedPointer<QueuePrivate> p;
};
