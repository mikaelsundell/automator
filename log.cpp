// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "log.h"

#include <QPointer>
#include <QSharedPointer>
#include <QTreeWidgetItem>
#include <QDebug>

// generated files
#include "ui_log.h"

class LogPrivate : public QObject
{
    Q_OBJECT
    public:
        LogPrivate();
        void init();
        void addJob(QSharedPointer<Job> job);
        void updateJob(const QUuid& uuid);
    
    public Q_SLOTS:
        void jobChanged();
        void selectionChanged();
        void clear();
        void close();

    public:
        QList<QSharedPointer<Job>> jobs; // prevent deletition
        QPointer<Log> dialog;
        QScopedPointer<Ui_Log> ui;
};

LogPrivate::LogPrivate()
{
    qRegisterMetaType<QSharedPointer<Job>>("QSharedPointer<Job>");
}

void
LogPrivate::init()
{
    // ui
    ui.reset(new Ui_Log());
    ui->setupUi(dialog);
    ui->items->setHeaderLabels(
        QStringList() << "Uuid"
                      << "Name"
                      << "Status"
    );
    // connect
    connect(ui->items, &QTreeWidget::itemSelectionChanged, this, &LogPrivate::selectionChanged);
    connect(ui->close, &QPushButton::pressed, this, &LogPrivate::close);
    connect(ui->clear, &QPushButton::pressed, this, &LogPrivate::clear);
}

void
LogPrivate::addJob(QSharedPointer<Job> job)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->items);
    item->setData(0, Qt::UserRole, QVariant::fromValue(job));
    ui->items->addTopLevelItem(item);
    updateJob(job->uuid());
    // connect
    connect(job.data(), SIGNAL(jobChanged()), this, SLOT(jobChanged()));
    jobs.append(job);
}

void
LogPrivate::updateJob(const QUuid& uuid)
{
    for (int i = 0; i < ui->items->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = ui->items->topLevelItem(i);
        QVariant data = item->data(0, Qt::UserRole);
        QSharedPointer<Job> itemjob = data.value<QSharedPointer<Job>>();
        
        if (itemjob->uuid() == uuid) {
            item->setText(0, itemjob->uuid().toString());
            item->setText(1, itemjob->name());
            switch(itemjob->status())
            {
                case Job::Pending: {
                    item->setText(2, "Pending");
                }
                break;
                case Job::Running: {
                    item->setText(2, "Running");
                }
                break;
                case Job::Completed: {
                    item->setText(2, "Completed");
                }
                break;
                case Job::Failed: {
                    item->setText(2, "Failed");
                }
                break;
                case Job::Cancelled: {
                    item->setText(2, "Cancelled");
                }
                break;
            }
        }
    }
}

void
LogPrivate::jobChanged()
{
    updateJob(qobject_cast<Job*>(sender())->uuid());
}

void
LogPrivate::selectionChanged()
{
    
    
    QList<QTreeWidgetItem*> selectedItems = ui->items->selectedItems();
    if (!selectedItems.isEmpty()) {
        // Get the first selected item
        QTreeWidgetItem* item = selectedItems.first();
        QVariant data = item->data(0, Qt::UserRole);
        QSharedPointer<Job> job = data.value<QSharedPointer<Job>>();
        
        ui->log->setText(job->log());
    
    }
}

void
LogPrivate::clear()
{
    ui->items->clear();
}

void
LogPrivate::close()
{
    dialog->close();
}

#include "log.moc"

Log::Log(QWidget* parent)
: QDialog(parent)
, p(new LogPrivate())
{
    p->dialog = this;
    p->init();
}

Log::~Log()
{
}

void
Log::addJob(QSharedPointer<Job> job)
{
    p->addJob(job);
}
