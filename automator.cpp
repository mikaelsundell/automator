// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "automator.h"
#include "dropfilter.h"
#include "error.h"
#include "eventfilter.h"
#include "icctransform.h"
#include "mac.h"
#include "monitor.h"
#include "preferences.h"
#include "preset.h"
#include "process.h"
#include "question.h"
#include "queue.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QDesktopServices>
#include <QList>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QTimer>
#include <QThread>
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
        void profile();
        void activate();
        void deactivate();
        void enable(bool enable);
        bool eventFilter(QObject* object, QEvent* event);
        void loadSettings();
        void saveSettings();
    
    public Q_SLOTS:
        void loadPresets();
        void togglePreset();
        void toggleFiledrop();
        void showMonitor();
        void run(const QList<QString>& files);
        void jobProcessed(const QUuid& uuid);
        void addFiles();
        void refreshPresets();
        void openPreset();
        void openPresetfrom();
        void openSaveto();
        void showSaveto();
        void setSaveto(const QString& text);
        void saveToChanged(const QString& text);
        void createFolderChanged(int state);
        void threadsChanged(int index);
        void showAbout();
        void showPreferences();
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
        QString replacePattern(const QString& input, const QString& pattern, const QFileInfo& inputinfo);
        QString replaceInput(const QString& input, const QFileInfo& inputinfo, const QFileInfo& outputinfo);
        int width;
        int height;
        QSize size;
        QString presetselected;
        QString presetfrom;
        QString saveto;
        QString filesfrom;
        bool createfolders;
        QMap<QString, QList<QUuid>> processedfiles;
        QPointer<Queue> queue;
        QPointer<Automator> window;
        QScopedPointer<About> about;
        QScopedPointer<Preferences> preferences;
        QScopedPointer<Monitor> monitor;
        QScopedPointer<Dropfilter> dropfilter;
        QScopedPointer<Eventfilter> presetfilter;
        QScopedPointer<Eventfilter> filedropfilter;
        QScopedPointer<Ui_Automator> ui;
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
    mac::setDarkAppearance();
    // icc profile
    ICCTransform* transform = ICCTransform::instance();
    QDir resources(QApplication::applicationDirPath() + "/../Resources");
    QString inputProfile = resources.filePath("sRGB2014.icc"); // built-in Qt input profile
    transform->setInputProfile(inputProfile);
    profile();
    // queue
    queue = Queue::instance();
    // ui
    ui.reset(new Ui_Automator());
    ui->setupUi(window);
    // about
    about.reset(new About(window.data()));
    // preferences
    preferences.reset(new Preferences(window.data()));
    // monitor
    monitor.reset(new Monitor(window.data()));
    monitor->setModal(false);
    // settings
    loadSettings();
    // layout
    // needed to keep .ui fixed size from setupUi
    window->setFixedSize(window->size());
    // event filter
    window->installEventFilter(this);
    // display filter
    presetfilter.reset(new Eventfilter);
    ui->presetBar->installEventFilter(presetfilter.data());
    // color filter
    filedropfilter.reset(new Eventfilter);
    ui->filedropBar->installEventFilter(filedropfilter.data());
    // drop filter
    dropfilter.reset(new Dropfilter);
    ui->saveTo->installEventFilter(dropfilter.data());
    // progress
    ui->fileprogress->hide();
    // connect
    connect(ui->togglePreset, &QPushButton::pressed, this, &AutomatorPrivate::togglePreset);
    connect(ui->toggleFiledrop, &QPushButton::pressed, this, &AutomatorPrivate::toggleFiledrop);
    connect(presetfilter.data(), &Eventfilter::pressed, ui->togglePreset, &QPushButton::click);
    connect(filedropfilter.data(), &Eventfilter::pressed, ui->toggleFiledrop, &QPushButton::click);
    connect(ui->addfiles, &QAction::triggered, this, &AutomatorPrivate::addFiles);
    connect(ui->refreshPresets, &QPushButton::clicked, this, &AutomatorPrivate::refreshPresets);
    connect(ui->openPreset, &QPushButton::clicked, this, &AutomatorPrivate::openPreset);
    connect(ui->openPresetfrom, &QPushButton::clicked, this, &AutomatorPrivate::openPresetfrom);
    connect(ui->openSaveto, &QPushButton::clicked, this, &AutomatorPrivate::openSaveto);
    connect(ui->showSaveto, &QPushButton::clicked, this, &AutomatorPrivate::showSaveto);
    connect(dropfilter.data(), &Dropfilter::textChanged, this, &AutomatorPrivate::saveToChanged);
    connect(ui->createFolders, &QCheckBox::stateChanged, this, &AutomatorPrivate::createFolderChanged);
    connect(ui->filedrop, &Filedrop::filesDropped, this, &AutomatorPrivate::run);
    connect(ui->threads, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AutomatorPrivate::threadsChanged);
    connect(ui->monitor, &QPushButton::clicked, this, &AutomatorPrivate::showMonitor);
    connect(ui->about, &QAction::triggered, this, &AutomatorPrivate::showAbout);
    connect(ui->preferences, &QAction::triggered, this, &AutomatorPrivate::showPreferences);
    connect(ui->openGithubReadme, &QAction::triggered, this, &AutomatorPrivate::openGithubReadme);
    connect(ui->openGithubIssues, &QAction::triggered, this, &AutomatorPrivate::openGithubIssues);
    connect(queue.data(), &Queue::jobProcessed, this, &AutomatorPrivate::jobProcessed);
    size = window->size();
    // threads
    int threads = QThread::idealThreadCount();
    for (int i = 1; i <= threads; ++i) {
        ui->threads->addItem(QString::number(i), i);
    }
    // cpu
    QTimer *timer = new QTimer(window.data());
    QObject::connect(timer, &QTimer::timeout, [&]() {
        if (ui->fileprogress->maximum()) {
            Process process;
            process.run("ps", QStringList() << "-A" << "-o" << "%cpu");
            process.wait();
            QString output = process.standardOutput();
            QStringList lines = output.split("\n", Qt::SkipEmptyParts);
            double totalCpu = 0;
            for (int i = 1; i < lines.size(); ++i) { // Starting at 1 skips the header line
                totalCpu += lines[i].trimmed().toDouble();
            }
            int numberOfCores = QThread::idealThreadCount();
            double normalizedCpuUsage = totalCpu / numberOfCores;
            ui->cpu->setText(QString("CPU: %1%").arg(normalizedCpuUsage, 0, 'f', 0));
        } else {
            ui->cpu->setText(QString(""));
        }
    });
    timer->start(1000);
    // stylesheet
    stylesheet();
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
    // presets
    QTimer::singleShot(0, [this]() { this->loadPresets(); });
}

void
AutomatorPrivate::stylesheet()
{ 
    QDir resources(QApplication::applicationDirPath());
    QFile stylesheet(resources.absolutePath() + "/../Resources/App.css");
    stylesheet.open(QFile::ReadOnly);
    QString qss = stylesheet.readAll();
    QRegularExpression hslRegex("hsl\\(\\s*(\\d+)\\s*,\\s*(\\d+)%\\s*,\\s*(\\d+)%\\s*\\)");
    QString transformqss = qss;
    QRegularExpressionMatchIterator i = hslRegex.globalMatch(transformqss);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            if (!match.captured(1).isEmpty() &&
                !match.captured(2).isEmpty() &&
                !match.captured(3).isEmpty())
            {
                int h = match.captured(1).toInt();
                int s = match.captured(2).toInt();
                int l = match.captured(3).toInt();
                QColor color = QColor::fromHslF(h / 360.0f, s / 100.0f, l / 100.0f);
                // icc profile
                ICCTransform* transform = ICCTransform::instance();
                color = transform->map(color.rgb());
                QString hsl = QString("hsl(%1, %2%, %3%)")
                                .arg(color.hue() == -1 ? 0 : color.hue())
                                .arg(static_cast<int>(color.hslSaturationF() * 100))
                                .arg(static_cast<int>(color.lightnessF() * 100));
                
                transformqss.replace(match.captured(0), hsl);
            }
        }
    }
    qApp->setStyleSheet(transformqss);
}

void
AutomatorPrivate::profile()
{
    QString outputProfile = mac::grabIccProfileUrl(window->winId());
    // icc profile
    ICCTransform* transform = ICCTransform::instance();
    transform->setOutputProfile(outputProfile);
}

void
AutomatorPrivate::activate()
{
    enable(true);
}

void
AutomatorPrivate::deactivate()
{
    enable(false);
}

void
AutomatorPrivate::addFiles()
{
    QStringList filenames = QFileDialog::getOpenFileNames(
                                window.data(),
                                tr("Select files"),
                                filesfrom,
                                tr("All files (*)")
    );

    if (!filenames.isEmpty()) {
        QFileInfo fileInfo(filenames.first());
        filesfrom = fileInfo.absolutePath();
        run(filenames);
    }
}

void
AutomatorPrivate::enable(bool enable)
{
    ui->openPreset->setEnabled(enable);
    ui->refreshPresets->setEnabled(enable);
    ui->filedrop->setEnabled(enable);
    ui->fileprogress->setEnabled(enable);
}

bool
AutomatorPrivate::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::ScreenChangeInternal) {
        profile();
        stylesheet();
    }
    if (event->type() == QEvent::Close) {
        if (ui->fileprogress->value()) {
            if (Question::askQuestion(window.data(), "Jobs are in progress, are you sure you want to quit?")) {
                saveSettings();
                return true;
            } else {
                event->ignore();
                return true;
            }
        }
        saveSettings();
        return true;
    }
    return QObject::eventFilter(object, event);
}

void
AutomatorPrivate::loadSettings()
{
    QDir applicationPath(QApplication::applicationDirPath());
    QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString presets = applicationPath.absolutePath() + "/../Presets";
    QSettings settings(MACOSX_BUNDLE_GUI_IDENTIFIER, "Automator");
    filesfrom = settings.value("filesFrom", documents).toString();
    presetselected = settings.value("presetselected", "").toString();
    presetfrom = settings.value("presetFrom", presets).toString();
    saveto = settings.value("saveTo", documents).toString();
    createfolders = settings.value("createFolders", false).toBool();
    // ui
    setSaveto(saveto);
    ui->createFolders->setChecked(createfolders);
}

void
AutomatorPrivate::saveSettings()
{
    QSettings settings(MACOSX_BUNDLE_GUI_IDENTIFIER, "Automator");
    settings.setValue("filesFrom", filesfrom);
    // presets
    if (ui->presets->count()) {
        QSharedPointer<Preset> preset = ui->presets->currentData().value<QSharedPointer<Preset>>();
        if (!preset.isNull()) { // skip "No presets found"
            settings.setValue("presetselected", preset->filename());
        }
    }
    settings.setValue("presetFrom", presetfrom);
    settings.setValue("saveTo", saveto);
    settings.setValue("createFolders", createfolders);
}

void
AutomatorPrivate::loadPresets()
{
    ui->presets->clear();
    QDir presets(presetfrom);
    QFileInfoList presetfiles = presets.entryInfoList(QStringList("*.json"));
    
    QString error;
    if (presetfiles.count() > 0) {
        for(QFileInfo presetfile : presetfiles) {
            QSharedPointer<Preset> preset(new Preset());
            QString filename = presetfile.absoluteFilePath();
            if (preset->read(filename)) {
                ui->presets->addItem(preset->name(), QVariant::fromValue(preset));
                if (filename == presetselected) {
                    ui->presets->setCurrentIndex(ui->presets->count() - 1);
                }
            } else {
                if (error.length() > 0) {
                    error += "\n";
                }
                error += QString("Could not load preset from file:\n"
                                 "%1\n\n"
                                 "Error:\n"
                                 "%2\n")
                                 .arg(presetfile.absoluteFilePath())
                                 .arg(preset->error());
            }
        }
        if (error.length() > 0) {
            Error::showError(window.data(), "Could not load all presets", error);
        }
        activate();
        
    } else {
        ui->presets->addItem("No presets found");
        deactivate();
    }
}

QString
AutomatorPrivate::replacePattern(const QString& input, const QString& pattern, const QFileInfo& fileinfo)
{
    QString result = input;
    QList<QPair<QString, QString>> replacements = {
        {QString("%%1dir%").arg(pattern), fileinfo.absolutePath()},
        {QString("%%1file%").arg(pattern), fileinfo.absoluteFilePath()},
        {QString("%%1ext%").arg(pattern), fileinfo.suffix()},
        {QString("%%1base%").arg(pattern), fileinfo.baseName()}
    };
    for (const auto& replacement : replacements) {
        result.replace(replacement.first, replacement.second);
    }
    return result;
}

QString
AutomatorPrivate::replaceInput(const QString& input, const QFileInfo& inputinfo, const QFileInfo& outputinfo)
{
    return replacePattern(replacePattern(input, "input", inputinfo), "output", outputinfo);
}

void
AutomatorPrivate::run(const QList<QString>& files)
{
    QSharedPointer<Preset> preset = ui->presets->currentData().value<QSharedPointer<Preset>>();
    QString outputDir = saveto;
    processedfiles.clear();
    int count = 0;
    for(const QString& file : files) {
        QMap<QString, QUuid> jobuuids;
        QList<QPair<QSharedPointer<Job>, QString>> dependentjobs;
        
        QFileInfo inputinfo(file);
        for(const Task& task : preset->tasks()) {
            QString extension = replacePattern(task.extension, "input", inputinfo);
            QString outputdir;
            if (createfolders) {
                outputdir =
                    outputDir +
                    "/" +
                    inputinfo.baseName();
            } else {
                outputdir = outputDir;
            }
            QString outputfile =
                outputdir +
                "/" +
                inputinfo.baseName() +
                "." +
                extension;
            QFileInfo outputinfo(outputfile);
            QString command = replaceInput(task.command, inputinfo, outputinfo);
            QStringList argumentlist = task.arguments.split(' ');
            for(QString& argument : argumentlist) {
                argument = replaceInput(argument, inputinfo, outputinfo);
            }
            QString startin = replaceInput(task.startin, inputinfo, outputinfo);
            QSharedPointer<Job> job(new Job());
            {
                job->setUuid(QUuid::createUuid());
                job->setId(task.id);
                job->setFilename(inputinfo.fileName());
                job->setName(task.name);
                job->setCommand(command);
                job->setArguments(argumentlist);
                job->setStartin(startin);
                job->setStatus(Job::Waiting);
            }
            job->setOutput(outputdir);
            if (task.dependson.isEmpty()) {
                QUuid uuid = queue->submit(job);
                count++;
                if (!processedfiles.contains(file)) {
                    processedfiles.insert(file, QList<QUuid>());
                }
                processedfiles[file].append(uuid);
                jobuuids[task.id] = uuid;
            } else {
                dependentjobs.append(qMakePair(job, task.dependson));
            }
    
        }
        for (QPair<QSharedPointer<Job>, QString> depedentjob : dependentjobs) {
            QSharedPointer<Job> job = depedentjob.first;
            QString dependentid = depedentjob.second;
            if (jobuuids.contains(dependentid)) {
                job->setDependson(jobuuids[dependentid]);
                QUuid uuid = queue->submit(job);
                if (!processedfiles.contains(file)) {
                    processedfiles.insert(file, QList<QUuid>());
                }
                processedfiles[file].append(uuid);
                jobuuids[job->id()] = uuid;
                count++;
            } else {
                QString status = QString("Status:\n"
                                         "Dependency not found for job: %1\n")
                                         .arg(job->name());
                job->setLog(status);
                job->setStatus(Job::Failed);
                return;
            }
        }
    }
    ui->fileprogress->setMaximum(ui->fileprogress->maximum() + count);
    if (!ui->fileprogress->isVisible()) {
        ui->fileprogress->show();
        ui->idleprogress->hide();
    }
}

void
AutomatorPrivate::jobProcessed(const QUuid& uuid)
{
    bool found = false;
    QStringList files = processedfiles.keys();
    for (const QString& file : files) {
        if (processedfiles[file].contains(uuid)) {
            processedfiles[file].removeAll(uuid);
            if (processedfiles[file].isEmpty()) {
                processedfiles.remove(file);
            }
            found = true;
        }
    }
    if (found) { // test if dropped
        int value = ui->fileprogress->value() + 1;
        if (value == ui->fileprogress->maximum()) {
            ui->fileprogress->setValue(0);
            ui->fileprogress->setMaximum(0);
            ui->fileprogress->hide();
            ui->idleprogress->show();
        } else {
            ui->fileprogress->setValue(value);
            ui->fileprogress->setToolTip(QString("Completed %1 of %2").arg(ui->fileprogress->value()).arg(ui->fileprogress->maximum()));
            
        }
    }
}

void
AutomatorPrivate::refreshPresets()
{
    loadPresets();
}

void
AutomatorPrivate::openPreset()
{
    if (ui->presets->count()) {
        QSharedPointer<Preset> preset = ui->presets->currentData().value<QSharedPointer<Preset>>();
        QDesktopServices::openUrl(QUrl::fromLocalFile(preset->filename()));
    }
}

void
AutomatorPrivate::openPresetfrom()
{
    QString dir = QFileDialog::getExistingDirectory(
                    window.data(),
                    tr("Open preset folder ..."),
                    presetfrom,
                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty()) {
        presetfrom = dir;
        refreshPresets();
    }
}

void
AutomatorPrivate::openSaveto()
{
    QString dir = QFileDialog::getExistingDirectory(
                    window.data(),
                    tr("Open savefolder ..."),
                    saveto,
                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    if (!dir.isEmpty()) {
        saveto = dir;
        setSaveto(saveto);
    }
}

void
AutomatorPrivate::showSaveto()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(saveto));
}

void
AutomatorPrivate::setSaveto(const QString& text)
{
    QFontMetrics metrics(ui->saveTo->font());
    ui->saveTo->setText(metrics.elidedText(text, Qt::ElideRight, ui->saveTo->maximumSize().width()));
}

void
AutomatorPrivate::saveToChanged(const QString& text)
{
    saveto = text;
}

void
AutomatorPrivate::createFolderChanged(int state)
{
    createfolders = (state == Qt::Checked);
}

void
AutomatorPrivate::threadsChanged(int index)
{
    queue->setThreads(ui->threads->itemText(index).toInt());
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
AutomatorPrivate::showMonitor()
{
    if (monitor->isVisible()) {
        monitor->raise();
    } else {
        monitor->show();
    }
}

void
AutomatorPrivate::showAbout()
{
    about->exec();
}

void
AutomatorPrivate::showPreferences()
{
    preferences->exec();
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
