// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once

#include "queue.h"

#include <QDialog>

class LogPrivate;
class Log : public QDialog
{
    Q_OBJECT
    public:
        Log(QWidget* parent = nullptr);
        virtual ~Log();
        void addJob(QSharedPointer<Job> job);
    
    private:
        QScopedPointer<LogPrivate> p;
};
