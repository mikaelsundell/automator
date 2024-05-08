// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once

#include <QList>
#include <QScopedPointer>
#include <QString>

class Task {
    public:
        Task() = default;
    
    public:
        QString id;
        QString name;
        QString command;
        QString extension;
        QString arguments;
        QString startin;
        QString dependson;
        QStringList documentation;
};

class PresetPrivate;
class Preset
{
    public:
        Preset();
        virtual ~Preset();
        bool read(const QString& filename);
        bool valid() const;
        QString error() const;
        QString filename() const;

    public:
        QString name() const;
        QList<Task> tasks();
    
    private:
        QScopedPointer<PresetPrivate> p;
};
