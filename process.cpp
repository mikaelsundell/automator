
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
        void standardOutput();
        void standardError();
    
    public:
        QString outputBuffer;
        QString errorBuffer;
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
}

void
ProcessPrivate::standardOutput()
{
    outputBuffer.append(process->readAllStandardOutput());
}

void
ProcessPrivate::standardError()
{
    errorBuffer.append(process->readAllStandardError());
}

#include "process.moc"

Process::Process()
: p(new ProcessPrivate())
{
}

Process::~Process()
{
}

bool
Process::run(const QString& command, const QStringList& arguments, const QString& startin)
{
    p->init();
    if (startin.length()) {
        p->process->setWorkingDirectory(startin);
    }
    p->process->start(command, arguments);
    return p->process->waitForFinished(-1) && p->process->exitStatus() == QProcess::NormalExit &&
           p->process->exitCode() == 0;
}

bool
Process::exists(const QString& command)
{
    p->init();
    p->process->start("which", QStringList() << command);
    return p->process->waitForFinished(-1) && p->process->exitStatus() == QProcess::NormalExit &&
           p->process->exitCode() == 0;
}

QString
Process::standardOutput() const
{
    return p->outputBuffer;
}

QString
Process::standardError() const
{
    return p->errorBuffer;
}

int
Process::exitCode() const
{
    return p->process->exitCode();
}

Process::Status
Process::exitStatus() const
{
    if (p->process->exitStatus() == QProcess::NormalExit) {
        return Process::Normal;
    } else {
        return Process::Crash;
    }
}

