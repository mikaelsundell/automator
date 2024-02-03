// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once
#include <QMainWindow>

class Eventfilter : public QObject
{
    Q_OBJECT
    public:
        Eventfilter(QObject* object = nullptr);
        virtual ~Eventfilter();

    Q_SIGNALS:
        void pressed();
    
    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;
};
