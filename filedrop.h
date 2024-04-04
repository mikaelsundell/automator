// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once
#include <QMainWindow>

class FiledropPrivate;
class Filedrop : public QWidget
{
    Q_OBJECT
    public:
        Filedrop(QWidget* parent = nullptr);
        virtual ~Filedrop();
    
    Q_SIGNALS:
        void filesDropped(const QList<QString>& files);
    
    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dropEvent(QDropEvent* event) override;

    private:
        QScopedPointer<FiledropPrivate> p;
};
