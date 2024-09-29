
// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "process.h"

#include <unistd.h>
#include <spawn.h>
#include <signal.h>
#include <crt_externs.h>
#include <sys/stat.h>

#include <QDir>
#include <QProcess>
#include <QThread>
#include <QDebug>

class ProcessPrivate : public QObject
{
    Q_OBJECT
    public:
        ProcessPrivate();
        ~ProcessPrivate();
        void init();
        void run(const QString& command, const QStringList& arguments, const QString& startin);
        bool wait();
        void kill();
        void kill(int pid);
    public:
        QString mapCommand(const QString& command);
        pid_t pid;
        int exitCode;
        QString outputBuffer;
        QString errorBuffer;
        bool running;
        int outputpipe[2];
        int errorpipe[2];
};

ProcessPrivate::ProcessPrivate()
: pid(-1)
, exitCode(-1)
, running(false)
{
}

ProcessPrivate::~ProcessPrivate()
{
    if (outputpipe[0] != -1) close(outputpipe[0]);
    if (errorpipe[0] != -1) close(errorpipe[0]);
}

void
ProcessPrivate::init()
{
}

void
ProcessPrivate::run(const QString& command, const QStringList& arguments, const QString& startin)
{
    running = false;
    outputBuffer.clear();
    errorBuffer.clear();
    QString absolutepath = mapCommand(command);
    QList<char *> argv;
    QByteArray commandbytes = absolutepath.toLocal8Bit();
    argv.push_back(commandbytes.data());
    std::vector<QByteArray> argbytes;
    for (const QString &arg : arguments) {
        argbytes.push_back(arg.toLocal8Bit());
        argv.push_back(argbytes.back().data());
    }
    argv.push_back(nullptr);

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    // Check if pipes are created successfully
    if (pipe(outputpipe) == -1 || pipe(errorpipe) == -1) {
        qDebug() << "Error creating pipes";
        return;  // Handle pipe creation failure
    }

    posix_spawn_file_actions_adddup2(&actions, outputpipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, errorpipe[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, outputpipe[0]);
    posix_spawn_file_actions_addclose(&actions, errorpipe[0]);

    if (!startin.isEmpty()) {
        chdir(startin.toLocal8Bit().data());
    }

    char** environ = *_NSGetEnviron();
    int status = posix_spawn(&pid, commandbytes.data(), &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);

    // Close write ends of pipes immediately after spawning the process
    close(outputpipe[1]);
    close(errorpipe[1]);

    if (status == 0) {
        running = true;
    } else {
        exitCode = -1;
        qDebug() << "Process failed to start";
    }
}

bool
ProcessPrivate::wait()
{
    if (running) {
        int status;
        waitpid(pid, &status, 0);
        running = false;
        char buffer[1024];
        ssize_t bytesread;
        if (outputpipe[0] != -1) {
            while ((bytesread = read(outputpipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytesread] = '\0';
                outputBuffer.append(buffer);
            }
            close(outputpipe[0]);
            outputpipe[0] = -1;
        }
        if (errorpipe[0] != -1) {
            while ((bytesread = read(errorpipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytesread] = '\0';
                errorBuffer.append(buffer);
            }
            close(errorpipe[0]);
            errorpipe[0] = -1;
        }
        if (WIFEXITED(status)) {
            exitCode = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exitCode = -WTERMSIG(status);
        } else {
            exitCode = -1;
        }
        return exitCode == 0;
    }
    return false;
}

void
ProcessPrivate::kill()
{
    if (running) {
        ::kill(pid, SIGKILL);
        wait();
    }
}

QString
ProcessPrivate::mapCommand(const QString& command)
{
    QString pathenv = QString::fromLocal8Bit(getenv("PATH"));
    QStringList paths = pathenv.split(':');
    for (const QString& path : paths) {
        QString fullPath = path + "/" + command;
        struct stat buffer;
        if (stat(fullPath.toLocal8Bit().data(), &buffer) == 0 && (buffer.st_mode & S_IXUSR)) {
            return fullPath;
        }
    }
    return command;
}

#include "process.moc"

Process::Process()
: p(new ProcessPrivate())
{
}

Process::~Process()
{
}

void
Process::run(const QString& command, const QStringList& arguments, const QString& startin)
{
    p->run(command, arguments, startin);
}

bool
Process::wait()
{
    return p->wait();
}

bool
Process::exists(const QString& command)
{
    Process process;
    process.run("which", QStringList() << command, "");
    return process.wait();
}

void
Process::kill()
{
    p->kill();
}

int
Process::pid() const
{
    return p->pid;
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
    return p->exitCode;
}

Process::Status
Process::exitStatus() const
{
    return (p->exitCode == 0) ? Process::Normal : Process::Crash;
}

void
Process::kill(int pid)
{
    ::kill(pid, SIGKILL);
}
