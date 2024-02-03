// Copyright 2022-present Contributors to the eventfilter project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/eventfilter

#include "eventfilter.h"

#include <QMouseEvent>
#include <QDebug>

Eventfilter::Eventfilter(QObject* parent)
: QObject(parent)
{
}

Eventfilter::~Eventfilter()
{
}

bool
Eventfilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            // Handle the left mouse button press event here
            emit pressed();
        }
    }

    // Continue processing the event
    return QObject::eventFilter(obj, event);
}
