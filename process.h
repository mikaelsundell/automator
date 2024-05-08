// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once

#include <QObject>

class ProcessPrivate;
class Process : public QObject {
    Q_OBJECT
    
    public:
        enum Status {
            Normal,
            Crash
        };
        Q_ENUM(Status)
    
    public:
        Process();
        virtual ~Process();
        bool run(const QString& command, const QStringList& arguments, const QString& startin);
        bool exists(const QString& command);
        QString standardOutput() const;
        QString standardError() const;
        int exitCode() const;
        Status exitStatus() const;
    
    private:
        QScopedPointer<ProcessPrivate> p;
};
