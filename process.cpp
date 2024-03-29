
// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "process.h"

#include <QProcess>

#include <QDebug>

class ProcessPrivate : public QObject
{
    Q_OBJECT
    public:
        ProcessPrivate();
        void init();
    
    public Q_SLOTS:
        QString standardOutput();
        QString standardError();
        void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    public:
        QScopedPointer<QProcess> process;
};

ProcessPrivate::ProcessPrivate()
{
}

void
ProcessPrivate::init()
{
    process.reset(new QProcess);
    // connect
    connect(process.data(), &QProcess::readyReadStandardOutput, this, &ProcessPrivate::standardOutput);
    connect(process.data(), &QProcess::readyReadStandardError, this, &ProcessPrivate::standardError);
    connect(process.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &ProcessPrivate::processFinished);
}

QString
ProcessPrivate::standardOutput()
{
    return process->readAllStandardOutput();
}

QString
ProcessPrivate::standardError()
{
    return process->readAllStandardError();
}

void
ProcessPrivate::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Process finished with exit code" << exitCode << "and exit status" << exitStatus;
}

#include "process.moc"

Process::Process()
: p(new ProcessPrivate())
{
    p->init();
}

Process::~Process()
{
}

void
Process::run(const QString& command, const QStringList& arguments)
{
    p->process->start(command, arguments);
    p->process->waitForFinished();
}

QString
Process::standardOutput() const
{
    return p->standardOutput();
}

QString
Process::standardError() const
{
    return p->standardError();
}
