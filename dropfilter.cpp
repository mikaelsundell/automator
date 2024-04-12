// Copyright 2022-present Contributors to the eventfilter project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/eventfilter

#include "dropfilter.h"

#include <QDragEnterEvent>
#include <QFileInfo>
#include <QLabel>
#include <QMimeData>
#include <QDebug>

Dropfilter::Dropfilter(QObject* parent)
: QObject(parent)
{
}

Dropfilter::~Dropfilter()
{
}

bool
Dropfilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::DragEnter) {
        QDragEnterEvent* dragevent = static_cast<QDragEnterEvent *>(event);
        if (dragevent->mimeData()->hasUrls()) {
            QList<QUrl> urls = dragevent->mimeData()->urls();
            if (!urls.isEmpty() && QFileInfo(urls.first().toLocalFile()).isDir()) {
                dragevent->acceptProposedAction();
                return true;
            }
        }
        return false;
    } else if (event->type() == QEvent::Drop) {
        QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
        QList<QUrl> urls = dropEvent->mimeData()->urls();
        if (!urls.isEmpty()) {
            QString dirpath = urls.first().toLocalFile();
            if (dirpath.endsWith("/")) {
                dirpath.chop(1);
            }
            QLabel* label = qobject_cast<QLabel *>(obj);
            if (label) {
                label->setText(dirpath);
                textChanged(dirpath);
                dropEvent->acceptProposedAction();
                return true;
            }
        }
        return false;
    }
    return QObject::eventFilter(obj, event);
}

