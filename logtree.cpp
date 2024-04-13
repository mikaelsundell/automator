// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "logtree.h"

#include <QPointer>
#include <QMouseEvent>
#include <QDebug>

class LogTreePrivate : public QObject
{
    Q_OBJECT
    public:
        LogTreePrivate();
        void init();

    public:
        QPointer<LogTree> widget;
};

LogTreePrivate::LogTreePrivate()
{
}

void
LogTreePrivate::init()
{
}

#include "logtree.moc"

LogTree::LogTree(QWidget* parent)
: QTreeWidget(parent)
, p(new LogTreePrivate())
{
    p->widget = this;
    p->init();
}

LogTree::~LogTree()
{
}

void
LogTree::mousePressEvent(QMouseEvent *event) {
    QTreeWidget::mousePressEvent(event);
    if (itemAt(event->pos()) == nullptr) {
        clearSelection();
    }
}
