// Copyright 2022-present Contributors to the automator project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#pragma once
#include <QWidget>

namespace mac
{
    struct IccProfile {
        int screenNumber;
        QString displayProfileUrl;
    };
    void setDarkAppearance();
    IccProfile grabIccProfile(WId wid);
    QString grabIccProfileUrl(WId wid);
}
