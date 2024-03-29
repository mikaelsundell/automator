// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once

#include "job.h"

#include <QObject>
#include <QScopedPointer>

class QueuePrivate;
class Queue : public QObject
{
    Q_OBJECT
    public:
        Queue();
        virtual ~Queue();
        QUuid submit(QSharedPointer<Job> job);
    
    Q_SIGNALS:
        void jobProcessed(const QUuid& uuid);

    private:
        QScopedPointer<QueuePrivate> p;
};
