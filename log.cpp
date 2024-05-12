// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "log.h"
#include "icctransform.h"

#include <QPainter>
#include <QPointer>
#include <QSharedPointer>
#include <QTreeWidgetItem>
#include <QStyledItemDelegate>
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
        bool eventFilter(QObject* object, QEvent* event);
    
    public Q_SLOTS:
        void jobChanged();
        void selectionChanged();
        void clear();
        void close();

    public:
        class StatusDelegate : public QStyledItemDelegate {
            public:
                StatusDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
                void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
                    QStyleOptionViewItem opt(option);
                    initStyleOption(&opt, index);
                    if (opt.state & QStyle::State_Selected) {
                        QStyledItemDelegate::paint(painter, option, index);
                    } else {
                        QColor color;
                        QString status = index.data().toString();
                        // icc profile
                        ICCTransform* transform = ICCTransform::instance();
                        if (status == "Dependency" ||
                            status == "Failed" ||
                            status == "Cancelled") {
                            color = transform->map(QColor::fromHsl(359, 90, 60).rgb());
                        } else if (status == "Pending") {
                                color = transform->map(QColor::fromHsl(309, 90, 60).rgb());
                        } else if (status == "Running" ||
                                   status == "Completed") {
                            color = transform->map(QColor::fromHsl(120, 90, 60).rgb());
                        } else {
                            color = Qt::transparent;
                        }
                        painter->save();
                        painter->setRenderHint(QPainter::Antialiasing, true);
                        
                        QFontMetrics metrics(painter->font());
                        int textWidth = metrics.horizontalAdvance(status);
                        int textHeight = metrics.height();
                        int leftPadding = 4;
                        QRect textRect(
                            option.rect.left() + leftPadding + 1,
                            (option.rect.center().y() - textHeight / 2) - 0,
                            textWidth, textHeight
                        );

                        painter->setBrush(color);
                        painter->setPen(Qt::NoPen);
                        painter->drawRoundedRect(textRect.adjusted(-5, -5, 5, 5), 8, 8);

                        // Draw text
                        painter->setPen(Qt::white);
                        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, status);
                        painter->restore();
                    }
                }
        };
        QTreeWidgetItem* findItemByUuid(const QUuid& uuid, QTreeWidgetItem* parent = nullptr);
        QSize size;
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
                      << "Filename"
                      << "Status"
    );
    ui->items->setColumnWidth(0, 280);
    ui->items->setColumnWidth(1, 250);
    ui->items->setColumnWidth(2, 125);
    ui->items->setColumnWidth(3, 50);
    ui->items->header()->setStretchLastSection(true);
    ui->items->setItemDelegateForColumn(3, new StatusDelegate(ui->items));
    // event filter
    dialog->installEventFilter(this);
    // layout
    // connect
    connect(ui->items, &QTreeWidget::itemSelectionChanged, this, &LogPrivate::selectionChanged);
    connect(ui->close, &QPushButton::pressed, this, &LogPrivate::close);
    connect(ui->clear, &QPushButton::pressed, this, &LogPrivate::clear);
}

void
LogPrivate::addJob(QSharedPointer<Job> job)
{
    QTreeWidgetItem* parentItem = nullptr;
    QUuid dependsonUuid = job->dependson();
    if (!dependsonUuid.isNull()) {
       parentItem = findItemByUuid(dependsonUuid);
    }
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setData(0, Qt::UserRole, QVariant::fromValue(job));
    if (parentItem) {
        parentItem->addChild(item);
    } else {
        ui->items->addTopLevelItem(item);
    }
    updateJob(job->uuid());
    // connect
    connect(job.data(), SIGNAL(jobChanged()), this, SLOT(jobChanged()));
    jobs.append(job);
}

void
LogPrivate::updateJob(const QUuid& uuid)
{
    QTreeWidgetItem* item = findItemByUuid(uuid);
    QVariant data = item->data(0, Qt::UserRole);
    QSharedPointer<Job> itemjob = data.value<QSharedPointer<Job>>();
    
    if (itemjob->uuid() == uuid) {
        item->setText(0, itemjob->uuid().toString());
        item->setText(1, itemjob->name());
        item->setText(2, itemjob->filename());
        switch(itemjob->status())
        {
            case Job::Pending: {
                item->setText(3, "Pending");
            }
            break;
            case Job::Running: {
                item->setText(3, "Running");
            }
            break;
            case Job::Completed: {
                item->setText(3, "Completed");
            }
            break;
            case Job::Dependency: {
                item->setText(3, "Dependency");
            }
            break;
            case Job::Failed: {
                item->setText(3, "Failed");
            }
            break;
            case Job::Cancelled: {
                item->setText(3, "Cancelled");
            }
            break;
        }
        if (item->isSelected()) {
            ui->log->setText(itemjob->log());
        }
    }
}

bool
LogPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::Show) {
        QList<int> sizes;
        int height = ui->splitter->height();
        int jobsHeight = height * 0.75;
        int logHeight = height - jobsHeight;
        sizes << jobsHeight << logHeight;
        ui->splitter->setSizes(sizes);
    }
    return false;
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
    ui->log->clear();
}

void
LogPrivate::close()
{
    dialog->close();
}

QTreeWidgetItem*
LogPrivate::findItemByUuid(const QUuid& uuid, QTreeWidgetItem* parent)
{
    QTreeWidgetItem* foundItem = nullptr;
    int itemCount = parent ? parent->childCount() : ui->items->topLevelItemCount();
    for (int i = 0; i < itemCount; ++i) {
        QTreeWidgetItem* item = parent ? parent->child(i) : ui->items->topLevelItem(i);
        QVariant data = item->data(0, Qt::UserRole);
        QSharedPointer<Job> itemJob = data.value<QSharedPointer<Job>>();
        if (itemJob && itemJob->uuid() == uuid) {
            return item;
        }
        if (item->childCount() > 0) {
            foundItem = findItemByUuid(uuid, item);
            if (foundItem) {
                return foundItem;
            }
        }
    }
    return nullptr;
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
