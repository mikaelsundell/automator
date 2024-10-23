// Copyright 2022-present Contributors to the jobman project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/jobman

#pragma once

#include <QDialog>

class ErrorPrivate;
class Error : public QDialog
{
    Q_OBJECT
    public:
        Error(QWidget* parent = nullptr);
        virtual ~Error();
        void setTitle(const QString& title);
        void setError(const QString& error);
    
        static bool showError(QWidget* parent, const QString& title, const QString& error);
    private:
        QScopedPointer<ErrorPrivate> p;
};
