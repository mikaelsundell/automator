// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/colorpicker

#pragma once
#include <QMainWindow>

class AutomatorPrivate;
class Automator : public QMainWindow
{
    Q_OBJECT
    public:
        Automator();
        virtual ~Automator();
    private:
        QScopedPointer<AutomatorPrivate> p;
};
