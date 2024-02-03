// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "automator.h"
#include "eventfilter.h"
#include "mac.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QDesktopServices>
#include <QPointer>
#include <QWindow>

#include <QDebug>

// generated files
#include "ui_about.h"
#include "ui_automator.h"

class AutomatorPrivate : public QObject
{
    Q_OBJECT
    public:
        AutomatorPrivate();
        void init();
        void stylesheet();
        bool eventFilter(QObject* object, QEvent* event);
    
    public Q_SLOTS:
        void togglePreset();
        void toggleFiledrop();
        void about();
        void openGithubReadme();
        void openGithubIssues();
    
    Q_SIGNALS:
        void readOnly(bool readOnly);

    public:
        class About : public QDialog
        {
            public: About(QWidget *parent = nullptr)
            : QDialog(parent)
            {
                QScopedPointer<Ui_About> about;
                about.reset(new Ui_About());
                about->setupUi(this);
                about->version->setText(MACOSX_BUNDLE_LONG_VERSION_STRING);
                about->copyright->setText(MACOSX_BUNDLE_COPYRIGHT);
                QString url = GITHUBURL;
                about->github->setText(QString("Github project: <a href='%1'>%1</a>").arg(url));
                about->github->setTextFormat(Qt::RichText);
                about->github->setTextInteractionFlags(Qt::TextBrowserInteraction);
                about->github->setOpenExternalLinks(true);
                QFile file(":/files/resources/Copyright.txt");
                file.open(QIODevice::ReadOnly | QIODevice::Text);
                QTextStream in(&file);
                QString text = in.readAll();
                file.close();
                about->licenses->setText(text);
            }
        };
        int width;
        int height;
        QSize size;
        QPointer<Automator> window;
        QScopedPointer<Eventfilter> presetfilter;
        QScopedPointer<Eventfilter> filedropfilter;
        QScopedPointer<Ui_Automator> ui;
};

AutomatorPrivate::AutomatorPrivate()
: width(128)
, height(128)
{
}

void
AutomatorPrivate::init()
{
    mac::setupMac();
    // ui
    ui.reset(new Ui_Automator());
    ui->setupUi(window);
    // layout
    // needed to keep .ui fixed size from setupUi
    window->setFixedSize(window->size());
    // presets
    QDir presets(QApplication::applicationDirPath() + "/../Presets");
    for(QFileInfo presetFile : presets.entryInfoList( QStringList( "*.json" )))
    {
        ui->preset->addItem
            ("Use preset: " + presetFile.baseName(), QVariant::fromValue(presetFile.filePath()));
    }
    
    
    ui->saveto->addItem("/Volumes/output");
    ui->saveas->setText("%filename%.%extension%");
    ui->filename->setText("filename.extension");
    
    // stylesheet
    stylesheet();
    // event filter
    window->installEventFilter(this);
    // display filter
    presetfilter.reset(new Eventfilter);
    ui->presetBar->installEventFilter(presetfilter.data());
    // color filter
    filedropfilter.reset(new Eventfilter);
    ui->filedropBar->installEventFilter(filedropfilter.data());
    // connect
    connect(ui->togglePreset, SIGNAL(pressed()), this, SLOT(togglePreset()));
    connect(ui->toggleFiledrop, SIGNAL(pressed()), this, SLOT(toggleFiledrop()));
    connect(presetfilter.data(), SIGNAL(pressed()), ui->togglePreset, SLOT(click()));
    connect(filedropfilter.data(), SIGNAL(pressed()), ui->toggleFiledrop, SLOT(click()));
    connect(ui->about, SIGNAL(triggered()), this, SLOT(about()));
    connect(ui->openGithubReadme, SIGNAL(triggered()), this, SLOT(openGithubReadme()));
    connect(ui->openGithubIssues, SIGNAL(triggered()), this, SLOT(openGithubIssues()));
    size = window->size();
    // debug
    #ifdef QT_DEBUG
        QMenu* menu = ui->menubar->addMenu("Debug");
        {
            QAction* action = new QAction("Reload stylesheet...", this);
            action->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S));
            menu->addAction(action);
            connect(action, &QAction::triggered, [&]() {
                this->stylesheet();
            });
        }
    #endif
}

void
AutomatorPrivate::stylesheet()
{
    QDir resources(QApplication::applicationDirPath());
    QFile stylesheet(resources.absolutePath() + "/../Resources/App.css");
    stylesheet.open(QFile::ReadOnly);
    qApp->setStyleSheet(stylesheet.readAll());
}

bool
AutomatorPrivate::eventFilter(QObject* object, QEvent* event)
{
    return false;
}

void
AutomatorPrivate::togglePreset()
{
    int height = ui->presetWidget->height(); 
    if (ui->togglePreset->isChecked()) {
        ui->togglePreset->setIcon(QIcon(":/icons/resources/Collapse.png"));
        ui->presetWidget->show();
        window->setFixedSize(window->width(), size.height() + height);
    } else {
        
        ui->togglePreset->setIcon(QIcon(":/icons/resources/Expand.png"));
        ui->presetWidget->hide();
        window->setFixedSize(window->width(), size.height() - height);
    }
    size = window->size();
}

void
AutomatorPrivate::toggleFiledrop()
{
    int height = ui->filedropWidget->height();
    if (ui->toggleFiledrop->isChecked()) {
        ui->toggleFiledrop->setIcon(QIcon(":/icons/resources/Collapse.png"));
        ui->filedropWidget->show();
        window->setFixedSize(window->width(), size.height() + height);
    } else {
        
        ui->toggleFiledrop->setIcon(QIcon(":/icons/resources/Expand.png"));
        ui->filedropWidget->hide();
        window->setFixedSize(window->width(), size.height() - height);
    }
    size = window->size();
}

void
AutomatorPrivate::about()
{
    QPointer<About> about = new About(window.data());
    about->exec();
}

void
AutomatorPrivate::openGithubReadme()
{
    QDesktopServices::openUrl(QUrl("https://github.com/mikaelsundell/automator/blob/master/README.md"));
}

void
AutomatorPrivate::openGithubIssues()
{
    QDesktopServices::openUrl(QUrl("https://github.com/mikaelsundell/automator/issues"));
}

#include "automator.moc"

Automator::Automator()
: QMainWindow(nullptr,
  Qt::WindowTitleHint |
  Qt::CustomizeWindowHint |
  Qt::WindowCloseButtonHint |
  Qt::WindowMinimizeButtonHint)
, p(new AutomatorPrivate())
{
    p->window = this;
    p->init();
}

Automator::~Automator()
{
}
