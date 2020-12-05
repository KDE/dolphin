/*
 * SPDX-FileCopyrightText: 2011 Vishesh Yadav <vishesh3y@gmail.com>
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-only
 */

#include "kversioncontrolplugin.h"

KVersionControlPlugin::KVersionControlPlugin(QObject* parent) :
    QObject(parent)
{
}

KVersionControlPlugin::~KVersionControlPlugin()
{
}

QString KVersionControlPlugin::localRepositoryRoot(const QString &/*directory*/) const
{
    return QString();
}
