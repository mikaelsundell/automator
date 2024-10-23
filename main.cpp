// Copyright 2022-present Contributors to the jobman project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/mikaelsundell/jobman

#include <QApplication>
#include "jobman.h"

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Jobman* jobman = new Jobman();
    jobman->show();
    return app.exec();
}
