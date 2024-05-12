// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QUuid>

class JobPrivate;
class Job : public QObject {
    Q_OBJECT
    
    public:
        enum Status {
            Pending,
            Running,
            Completed,
            Dependency,
            Failed,
            Cancelled
        };
        Q_ENUM(Status)

    public:
        Job();
        virtual ~Job();
        QDateTime created() const;
        QString id() const;
        QString command() const;
        QStringList arguments() const;
        QUuid dependson() const;
        QString log() const;
        QString name() const;
        QUuid uuid() const;
        QString startin() const;
        Status status() const;
        void setId(const QString& id);
        void setCommand(const QString& command);
        void setArguments(const QStringList& arguments);
        void setDependson(QUuid dependson);
        void setLog(const QString& log);
        void setName(const QString& name);
        void setStartin(const QString& startin);
        void setStatus(Status status);
        void setUuid(QUuid uuid);
    
    Q_SIGNALS:
        void jobChanged();
    
    private:
        QScopedPointer<JobPrivate> p;
};
