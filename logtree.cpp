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
        bool eventFilter(QObject* object, QEvent* event);

    public:
        QPointer<LogTree> widget;
};

LogTreePrivate::LogTreePrivate()
{
}

void
LogTreePrivate::init()
{
    // event filter
    widget->installEventFilter(this);
}

bool
LogTreePrivate::eventFilter(QObject* object, QEvent* event)
{
    qDebug() << "event:" << event;
    
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(object);
        if (treeWidget && mouseEvent) {
            if (treeWidget->itemAt(mouseEvent->pos()) == nullptr) {
                treeWidget->clearSelection();
                return true;
            }
        }
    }
    return QObject::eventFilter(object, event);
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
