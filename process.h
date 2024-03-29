// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once

#include <QObject>

class ProcessPrivate;
class Process : public QObject {
    Q_OBJECT
    
    public:
        Process();
        virtual ~Process();
        void run(const QString& command, const QStringList& arguments);
        QString standardOutput() const;
        QString standardError() const;
    
    private:
        QScopedPointer<ProcessPrivate> p;
};
