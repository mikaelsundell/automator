// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include "preset.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include <QDebug>

class PresetPrivate : public QObject
{
    public:
        PresetPrivate();
        void init();
        bool read(const QString& filename);
    
    public:
        QString filename;
        QString log;
        QString name;
        QList<QString> description;
        QList<Task> tasks;
        bool valid;
};

PresetPrivate::PresetPrivate()
: valid(false)
{
}

void
PresetPrivate::init()
{
}

bool
PresetPrivate::read(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        log = QString("failed to open file: %1").arg(filename);
        valid = false;
        return valid;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument document = QJsonDocument::fromJson(jsonData);
    if (document.isNull()) {
        log = QString("failed to create json document for file: %1").arg(filename);
        valid = false;
        return valid;
    }
    if (!document.isObject()) {
        log = QString("json document is not an object for file: %1").arg(filename);
        valid = false;
        return valid;
    }
    
    QJsonObject json = document.object();
    if (json.contains("name") && json["name"].isString()) {
        name = json["name"].toString();
    }
    if (json.contains("tasks") && json["tasks"].isArray()) {
        QJsonArray tasksArray = json["tasks"].toArray();
        for (int i = 0; i < tasksArray.size(); ++i) {
            QJsonObject jsontask = tasksArray[i].toObject();
            Task task;
            if (jsontask.contains("id") && jsontask["id"].isString()) task.id = jsontask["id"].toString();
            if (jsontask.contains("name") && jsontask["name"].isString()) task.name = jsontask["name"].toString();
            if (jsontask.contains("command") && jsontask["command"].isString()) task.command = jsontask["command"].toString();
            if (jsontask.contains("extension") && jsontask["extension"].isString()) task.extension = jsontask["extension"].toString();
            if (jsontask.contains("arguments") && jsontask["arguments"].isString()) task.arguments = jsontask["arguments"].toString();
            if (jsontask.contains("startin") && jsontask["startin"].isString()) task.startin = jsontask["startin"].toString();
            if (jsontask.contains("dependson") && jsontask["dependson"].isString()) task.dependson = jsontask["dependson"].toString();
            if (jsontask.contains("documentation") && jsontask["documentation"].isArray()) {
                QJsonArray docarray = jsontask["documentation"].toArray();
                for (int i = 0; i < docarray.size(); ++i) {
                    task.documentation.append(docarray[i].toString());
                }
            }
            if (!task.name.isEmpty() && !task.command.isEmpty() && !task.extension.isEmpty() && !task.arguments.isEmpty()) {
                tasks.append(task);
            } else {
                log = QString("json for task: %1 is not valid, must contain name, extension and arguments").arg(i);
                
                qDebug() << "id: " << task.id;
                qDebug() << "name: " << task.name;
                qDebug() << "command: " << task.command;
                qDebug() << "extension: " << task.extension;
                qDebug() << "arguments: " << task.arguments;
                
                qDebug() << "log: " << log;
            }
        }
    }
    valid = true;
    return valid;
}


Preset::Preset()
: p(new PresetPrivate())
{
    p->init();
}

Preset::~Preset()
{
}

bool
Preset::read(const QString& filename)
{
    return p->read(filename);
}

bool
Preset::valid() const
{
    return p->valid;
}

QString
Preset::name() const
{
    return p->name;
}

QList<Task>
Preset::tasks()
{
    return p->tasks;
}
