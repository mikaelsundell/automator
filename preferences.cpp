// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "preferences.h"

#include <QFileDialog>
#include <QPointer>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>

// generated files
#include "ui_preferences.h"

class PreferencesPrivate : public QObject
{
    Q_OBJECT
    public:
        PreferencesPrivate();
        void init();
        bool eventFilter(QObject* object, QEvent* event);
        void loadSettings();
        void saveSettings();

    public Q_SLOTS:
        void selectionChanged();
        void add();
        void remove();
        void close();

    public:
        QString searchpathfrom;
        QStringList searchpaths;
        QPointer<Preferences> dialog;
        QScopedPointer<Ui_Preferences> ui;
};

PreferencesPrivate::PreferencesPrivate()
{
}

void
PreferencesPrivate::init()
{
    // ui
    ui.reset(new Ui_Preferences());
    ui->setupUi(dialog);
    // settings
    loadSettings();
    // event filter
    dialog->installEventFilter(this);
    // layout
    for(QString searchpath : searchpaths) {
        ui->searchpaths->addItem(searchpath);
    }
    // connect
    connect(ui->searchpaths, &QListWidget::itemSelectionChanged, this, &PreferencesPrivate::selectionChanged);
    connect(ui->add, &QPushButton::pressed, this, &PreferencesPrivate::add);
    connect(ui->remove, &QPushButton::pressed, this, &PreferencesPrivate::remove);
    connect(ui->close, &QPushButton::pressed, this, &PreferencesPrivate::close);
}

bool
PreferencesPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::Hide) {
        saveSettings();
    }
    return false;
}

void
PreferencesPrivate::loadSettings()
{
    QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QSettings settings(MACOSX_BUNDLE_GUI_IDENTIFIER, "Automator");
    searchpathfrom = settings.value("searchpathFrom", documents).toString();
    searchpaths = settings.value("searchpaths", documents).toStringList();
}

void
PreferencesPrivate::saveSettings()
{
    QSettings settings(MACOSX_BUNDLE_GUI_IDENTIFIER, "Automator");
    settings.setValue("searchpathFrom", searchpathfrom);
    searchpaths.clear();
    for (int i = 0; i < ui->searchpaths->count(); ++i) {
        QListWidgetItem* item = ui->searchpaths->item(i);
        if (item) {
            searchpaths.append(item->text());
        }
    }
    settings.setValue("searchpaths", searchpaths);
}

void
PreferencesPrivate::selectionChanged()
{
    QList<QListWidgetItem*> selectedItems = ui->searchpaths->selectedItems();
    if (!selectedItems.isEmpty()) {
        ui->remove->setEnabled(true);
    } else {
        ui->remove->setEnabled(false);
    }
}

void
PreferencesPrivate::add()
{
    QString dir = QFileDialog::getExistingDirectory(
                    dialog.data(),
                    tr("Add folder"),
                    searchpathfrom,
                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty()) {
        bool found = false;
        for (int i = 0; i < ui->searchpaths->count(); ++i) {
            QListWidgetItem *item = ui->searchpaths->item(i);
            if (item->text() == dir) {
                found = true;
                break;
            }
        }
        if (!found) {
            ui->searchpaths->addItem(dir);
            searchpathfrom = dir;
        }
    }
}

void
PreferencesPrivate::remove()
{
    QListWidget* searchpath = ui->searchpaths;
    QString dir = searchpath->currentItem()->text();
    if (!dir.isEmpty()) {
        QList<QListWidgetItem*> items = searchpath->findItems(dir, Qt::MatchExactly);
        foreach (QListWidgetItem* item, items) {
            delete searchpath->takeItem(ui->searchpaths->row(item));
        }
    }
}


void
PreferencesPrivate::close()
{
    dialog->close();
}

#include "preferences.moc"

Preferences::Preferences(QWidget* parent)
: QDialog(parent)
, p(new PreferencesPrivate())
{
    p->dialog = this;
    p->init();
}

Preferences::~Preferences()
{
}
