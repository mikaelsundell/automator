// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "error.h"

#include <QPointer>

// generated files
#include "ui_error.h"

class ErrorPrivate : public QObject
{
    Q_OBJECT
    public:
        ErrorPrivate();
        void init();
    
    public:
        QPointer<Error> dialog;
        QScopedPointer<Ui_Error> ui;
};

ErrorPrivate::ErrorPrivate()
{
}

void
ErrorPrivate::init()
{
    // ui
    ui.reset(new Ui_Error());
    ui->setupUi(dialog);
    // connect
    connect(ui->close, &QPushButton::clicked, this, [this]() {
       dialog->done(QDialog::Accepted);
    });
}

#include "error.moc"

Error::Error(QWidget* parent)
: QDialog(parent)
, p(new ErrorPrivate())
{
    p->dialog = this;
    p->init();
}

Error::~Error()
{
}

void
Error::setTitle(const QString& title)
{
    p->ui->title->setText(title);
}

void
Error::setError(const QString& error)
{
    p->ui->error->setPlainText(error);
}

bool
Error::showError(QWidget* parent, const QString& title, const QString& error)
{
    Error dialog(parent);
    dialog.setTitle(title);
    dialog.setError(error);
    const int result = dialog.exec();
    return result == QDialog::Accepted;
}

