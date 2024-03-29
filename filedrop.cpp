// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "filedrop.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QTimer>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QWidget>
#include <QPointer>
#include <QStyle>

#include <QDebug>

class FiledropPrivate : public QObject
{
    Q_OBJECT
    public:
        FiledropPrivate();
        void init();
        void update();

    public:
        int counter;
        bool running;
    
        QList<QString> files;
    
        QPointer<QLabel> label;
        QPointer<QProgressBar> progressBar;
        QPointer<QTimer> timer;
    
    
    
        QPointer<Filedrop> widget;
};

FiledropPrivate::FiledropPrivate()
: counter(0)
, running(false)
{
}

void 
FiledropPrivate::init()
{
    widget->setAcceptDrops(true);
    widget->setAttribute(Qt::WA_StyledBackground, true); // needed for stylesheet
    
    
    
    
    // layout
    
    QVBoxLayout* layout = new QVBoxLayout(widget);

    label = new QLabel(widget);
    QPixmap pixmap(":/icons/resources/Arrow.png"); // Load the pixmap directly from the resource file
    label->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation)); // Set the scaled pixmap on the label
    label->setFixedSize(64, 64); // Set the label's fixed size
    layout->addWidget(label, 0, Qt::AlignHCenter);
    

    progressBar = new QProgressBar(widget);

    progressBar->setMinimum(0);
    progressBar->setValue(0);
    progressBar->setMaximum(10);
    
    layout->addWidget(progressBar, 0, Qt::AlignHCenter);
    
    progressBar->hide();
    

    
    
}

void
FiledropPrivate::update()
{
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

#include "filedrop.moc"

Filedrop::Filedrop(QWidget* parent)
: QWidget(parent)
, p(new FiledropPrivate())
{
    p->widget = this;
    p->init();
}

Filedrop::~Filedrop()
{
}

void
Filedrop::dragEnterEvent(QDragEnterEvent* event)
{
    Q_UNUSED(event);
    qDebug() << "dragEnterEvent";
    
    if (!p->running) {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
            qDebug() << "accept: dragEnterEvent";
            
            p->files.clear();
            QList<QUrl> urls = event->mimeData()->urls();
            for (const QUrl &url : urls) {
                p->files.append(url.toLocalFile());
            }

            
            setProperty("dragging", true);
            p->update();
        }
    }
}

void
Filedrop::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event);
    qDebug() << "dragLeaveEvent";
    setProperty("dragging", false);
    p->update();
}

void
Filedrop::dropEvent(QDropEvent* event)
{
    Q_UNUSED(event);
    qDebug() << "dropEvent";
    setProperty("dragging", false);
    p->progressBar->show();
    p->label->hide();
    
    p->progressBar->setMinimum(0);
    p->progressBar->setValue(0);
    p->progressBar->setMaximum(p->files.count());
    filesDropped(p->files);
    
    p->running = true;
    p->update();
}

void
Filedrop::setProgress(int value)
{
    if (value > 0) {
        
        //qDebug() << "set value: " << value << " of max: " << p->progressBar->maximum();
        
        p->progressBar->setValue(p->progressBar->maximum() - value);
        
    } else {
        
        p->progressBar->hide();
        p->label->show();
        p->running = false;
        
    }
}




