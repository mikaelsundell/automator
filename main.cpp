// Copyright 2022-present Contributors to the colorpicker project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/automator

#include <QApplication>
#include "automator.h"

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Automator* automator = new Automator();
    automator->show();
    return app.exec();
}
