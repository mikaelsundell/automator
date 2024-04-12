// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once

#include <QTreeWidget>

class LogTreePrivate;
class LogTree : public QTreeWidget
{
    Q_OBJECT
    public:
        LogTree(QWidget* parent = nullptr);
        virtual ~LogTree();
    
    protected:
        void mousePressEvent(QMouseEvent *event) override;
    
    private:
        QScopedPointer<LogTreePrivate> p;
};
