/*
 * Copyright 2015 Ashish Bansal<bansal.ashish096@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <QApplication>
#include <QIcon>

#include <KRun>

#include "global.h"
#include "dolphindebug.h"

#include "dolphin_generalsettings.h"

QList<QUrl> Dolphin::validateUris(const QStringList& uriList)
{
    const QString currentDir = QDir::currentPath();
    QList<QUrl> urls;
    foreach (const QString& str, uriList) {
        const QUrl url = QUrl::fromUserInput(str, currentDir, QUrl::AssumeLocalFile);
        if (url.isValid()) {
            urls.append(url);
        } else {
            qCWarning(DolphinDebug) << "Invalid URL: " << str;
        }
    }
    return urls;
}

QUrl Dolphin::homeUrl()
{
    return QUrl::fromUserInput(GeneralSettings::homeUrl(), QString(), QUrl::AssumeLocalFile);
}

void Dolphin::openNewWindow(const QList<QUrl> &urls, QWidget *window, const OpenNewWindowFlags &flags)
{
    QString command = QStringLiteral("dolphin");

    if (flags.testFlag(OpenNewWindowFlag::Select)) {
        command.append(QLatin1String(" --select"));
    }

    if (!urls.isEmpty()) {
        command.append(QLatin1String(" %U"));
    }

    KRun::run(command, urls, window, qApp->applicationDisplayName(), qApp->windowIcon().name());
}
