// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "automator.h"
#include "eventfilter.h"
#include "log.h"
#include "mac.h"
#include "preset.h"
#include "queue.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QDesktopServices>
#include <QList>
#include <QPointer>
#include <QSharedPointer>
#include <QStandardPaths>
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
        void showLog();
        void run(const QList<QString>& files);
        void jobProcessed(const QUuid& uuid);
        void openSaveto();
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
        QString replacePattern(const QString& file, const QString& pattern, const QFileInfo& inputinfo);
        QString replaceFile(const QString& file, const QFileInfo& inputinfo, const QFileInfo& outputinfo);
        int width;
        int height;
        QSize size;
        QMap<QString, QList<QUuid>> processedfiles;
        QPointer<Automator> window;
        QScopedPointer<Log> log;
        QScopedPointer<Eventfilter> presetfilter;
        QScopedPointer<Eventfilter> filedropfilter;
        QScopedPointer<Ui_Automator> ui;
        QScopedPointer<Queue> queue;
};

AutomatorPrivate::AutomatorPrivate()
: width(128)
, height(128)
{
    qRegisterMetaType<QSharedPointer<Preset>>("QSharedPointer<Preset>");
}

void
AutomatorPrivate::init()
{
    mac::setupMac();
    // ui
    ui.reset(new Ui_Automator());
    ui->setupUi(window);
    // queue
    queue.reset(new Queue());
    // log
    log.reset(new Log(window.data()));
    log->setModal(false);
    // layout
    // needed to keep .ui fixed size from setupUi
    window->setFixedSize(window->size());
    // presets
    QDir presets(QApplication::applicationDirPath() + "/../Presets");
    for(QFileInfo presetFile : presets.entryInfoList( QStringList( "*.json" )))
    {
        QSharedPointer<Preset> preset(new Preset());
        if (preset->read(presetFile.absoluteFilePath()))
        {
            ui->preset->addItem(preset->name(), QVariant::fromValue(preset));
        }
    }
    
    ui->saveto->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
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
    connect(ui->togglePreset, &QPushButton::pressed, this, &AutomatorPrivate::togglePreset);
    connect(ui->toggleFiledrop, &QPushButton::pressed, this, &AutomatorPrivate::toggleFiledrop);
    connect(presetfilter.data(), &Eventfilter::pressed, ui->togglePreset, &QPushButton::click);
    connect(filedropfilter.data(), &Eventfilter::pressed, ui->toggleFiledrop, &QPushButton::click);
    connect(ui->filedrop, &Filedrop::filesDropped, this, &AutomatorPrivate::run);
    connect(ui->log, &QPushButton::clicked, this, &AutomatorPrivate::showLog);
    connect(ui->openSaveto, &QPushButton::clicked, this, &AutomatorPrivate::openSaveto);
    connect(ui->about, &QAction::triggered, this, &AutomatorPrivate::about);
    connect(ui->openGithubReadme, &QAction::triggered, this, &AutomatorPrivate::openGithubReadme);
    connect(ui->openGithubIssues, &QAction::triggered, this, &AutomatorPrivate::openGithubIssues);
    connect(queue.data(), &Queue::jobProcessed, this, &AutomatorPrivate::jobProcessed);
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

QString
AutomatorPrivate::replacePattern(const QString& file, const QString& pattern, const QFileInfo& fileinfo)
{
    QString string = file;
    {
        string.replace(QString("%%1dir%").arg(pattern), fileinfo.absolutePath());
        string.replace(QString("%%1file%").arg(pattern), fileinfo.absoluteFilePath());
        string.replace(QString("%%1ext%").arg(pattern), fileinfo.suffix());
        string.replace(QString("%%1base%").arg(pattern), fileinfo.baseName());
    }
    return string;
}

QString
AutomatorPrivate::replaceFile(const QString& file, const QFileInfo& inputinfo, const QFileInfo& outputinfo)
{
    return replacePattern(replacePattern(file, "input", inputinfo), "output", outputinfo);
}

void
AutomatorPrivate::run(const QList<QString>& files)
{
    QSharedPointer<Preset> preset = ui->preset->currentData().value<QSharedPointer<Preset>>();
    QString outputDir = ui->saveto->text();
    processedfiles.clear();
    for(const QString& file : files) {

        QMap<QString, QUuid> jobuuids;
        QList<QPair<QSharedPointer<Job>, QString>> dependentjobs;
        
        QFileInfo inputinfo(file);
        for(const Task& task : preset->tasks()) {
            QString extension = replacePattern(task.extension, "input", inputinfo);
            QString outputfile =
                outputDir +
                "/" +
                inputinfo.baseName() +
                "." +
                extension;
            
            QFileInfo outputinfo(outputfile);
            QString command = replaceFile(task.command, inputinfo, outputinfo);
            QString arguments = replaceFile(task.arguments, inputinfo, outputinfo);
            QString startin = replaceFile(task.startin, inputinfo, outputinfo);
            {
                QSharedPointer<Job> job(new Job());
                job->setName(task.name);
                job->setCommand(command);
                job->setArguments(arguments);
                job->setStartin(startin);
                
                
                if (task.dependson.isEmpty()) {
                    QUuid uuid = queue->submit(job);
                    log->addJob(job);
                    
                    if (!processedfiles.contains(file)) {
                        processedfiles.insert(file, QList<QUuid>());
                    }
                    processedfiles[file].append(uuid);
                    jobuuids[task.id] = uuid;
                } else {
                    dependentjobs.append(qMakePair(job, task.dependson));
                }
                
            }
        }
        for (QPair<QSharedPointer<Job>, QString> depedentjob : dependentjobs) {
            QSharedPointer<Job> job = depedentjob.first;
            QString dependentid = depedentjob.second;
            if (jobuuids.contains(dependentid)) {
                job->setDependson(jobuuids[dependentid]);
                if (!processedfiles.contains(file)) {
                    processedfiles.insert(file, QList<QUuid>());
                }
                processedfiles[file].append(queue->submit(job));
                log->addJob(job);
                
            } else {
                //qDebug() << "Dependency not found for job: " << job->name();
            }
        }
    }
}

void
AutomatorPrivate::jobProcessed(const QUuid& uuid)
{
    QStringList files = processedfiles.keys();
    for (const QString& file : files) {
        if (processedfiles[file].contains(uuid)) {
            processedfiles[file].removeAll(uuid);
            if (processedfiles[file].isEmpty()) {
                processedfiles.remove(file);
            }
        }
    }
    ui->filedrop->setProgress(processedfiles.count());
}

void
AutomatorPrivate::openSaveto()
{
    QString dir = QFileDialog::getExistingDirectory(
                    window.data(),
                    tr("Output directory"),
                    ui->saveto->text(),
                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty()) {
        ui->saveto->setText(dir);
    }
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
AutomatorPrivate::showLog()
{
    log->show();
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
