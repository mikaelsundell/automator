// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "queue.h"

#include <QObject>
#include <QPointer>

#include <QDebug>

class JobPrivate : public QObject
{
    Q_OBJECT
    public:
        JobPrivate();
        void init();

    public:
        QUuid uuid;
        QString name;
        QString command;
        QString arguments;
        QString startin;
        QUuid dependson;
        QString log;
        Job::Status status;
        QPointer<Job> job;
};

JobPrivate::JobPrivate()
: status(Job::Pending)
{
}

void
JobPrivate::init()
{
}

#include "job.moc"

Job::Job()
: p(new JobPrivate())
{
    p->job = this;
    p->init();
}

Job::~Job()
{
}

QString
Job::command() const
{
    return p->command;
}

QString
Job::arguments() const
{
    return p->arguments;
}

QUuid
Job::dependson() const
{
    return p->dependson;
}

QString
Job::log() const
{
    return p->log;
}

QString
Job::name() const
{
    return p->name;
}

QUuid
Job::uuid() const
{
    return p->uuid;
}

QString
Job::startin() const
{
    return p->startin;
}

Job::Status
Job::status() const
{
    return p->status;
}

void
Job::setCommand(const QString& command)
{
    if (p->command != command) {
        p->command = command;
        jobChanged();
    }
}

void
Job::setArguments(const QString& arguments)
{
    if (p->arguments != arguments) {
        p->arguments = arguments;
        jobChanged();
    }
}

void
Job::setDependson(QUuid dependson)
{
    if (p->dependson != dependson) {
        p->dependson = dependson;
        jobChanged();
    }
}

void
Job::setLog(const QString& log)
{
    if (p->log != log) {
        p->log = log;
        jobChanged();
    }
}

void
Job::setName(const QString& name)
{
    if (p->name != name) {
        p->name = name;
        jobChanged();
    }
}

void
Job::setStartin(const QString& startin)
{
    if (p->startin != startin) {
        p->startin = startin;
        jobChanged();
    }
}

void
Job::setStatus(Status status)
{
    if (p->status != status) {
        p->status = status;
        jobChanged();
    }
}

void
Job::setUuid(QUuid uuid)
{
    if (p->uuid != uuid) {
        p->uuid = uuid;
        jobChanged();
    }
}
