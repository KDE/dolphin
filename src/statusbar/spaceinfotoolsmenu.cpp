/***************************************************************************
 *   Copyright (C) 2014 by Gregor Mi <codestruct@posteo.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "spaceinfotoolsmenu.h"

#include <QAction>
#include <QUrl>

#include <KMountPoint>
#include <KLocalizedString>
#include <KRun>
#include <KService>

SpaceInfoToolsMenu::SpaceInfoToolsMenu(QWidget* parent, QUrl url)
    : QMenu(parent)
{
    const QString notInstalled = " [" + i18nc("@action:inmenu", "not installed") + "]";

    // find service
    //
    const auto filelightService = KService::serviceByDesktopName("filelight");
    if (filelightService && filelightService->isApplication()) {
        const auto filelightIcon = QIcon::fromTheme(filelightService->icon());

        if (url.isLocalFile()) { // 2015-01-12: Filelight can handle FTP connections but KIO/kioexec cannot (bug or feature?), so we don't offer it in this case
            // add action and connect signals
            //
            const auto startFilelightDirectoryAction = addAction(filelightService->genericName() + " - "
                    + i18nc("@action:inmenu", "current folder"));
            startFilelightDirectoryAction->setIcon(filelightIcon);

            connect(startFilelightDirectoryAction, &QAction::triggered, this, [filelightService, url](bool) {
                KRun::runService(*filelightService, { url }, nullptr);
            });
        }

        if (url.isLocalFile()) { // makes no sense for non-local URLs (e.g. FTP server), so we don't offer it in this case
            // add action and connect signals
            //
            const auto startFilelightDeviceAction = addAction(filelightService->genericName() + " - "
                                                    + i18nc("@action:inmenu", "current device"));
            startFilelightDeviceAction->setIcon(filelightIcon);

            connect(startFilelightDeviceAction, &QAction::triggered, this, [filelightService, url](bool) {
                KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByPath(url.toLocalFile());
                KRun::runService(*filelightService, { mountPoint->mountPoint() }, nullptr);
            });
        }

        // add action and connect signals
        //
        const auto startFilelightAllDevicesAction = addAction(filelightService->genericName() + " - "
                + i18nc("@action:inmenu", "all devices"));
        startFilelightAllDevicesAction->setIcon(filelightIcon);

        connect(startFilelightAllDevicesAction, &QAction::triggered, this, [filelightService](bool) {
            KRun::runService(*filelightService, { }, nullptr);
        });
    } else {
        const auto startFilelightDirectoryAction = addAction("Filelight" + notInstalled);
        startFilelightDirectoryAction->setEnabled(false);
    }

    // find service
    //
    const auto kdiskfreeService = KService::serviceByDesktopName("kdf");
    if (kdiskfreeService && kdiskfreeService->isApplication()) {
        //
        // add action and connect signals
        //
        const auto startKDiskFreeAction = addAction(kdiskfreeService->genericName());
        startKDiskFreeAction->setIcon(QIcon::fromTheme(kdiskfreeService->icon()));

        connect(startKDiskFreeAction, &QAction::triggered, this, [kdiskfreeService](bool) {
            KRun::runService(*kdiskfreeService, { }, nullptr);
        });
    } else {
        const auto startKDiskFreeAction = addAction("KDiskFree" + notInstalled);
        startKDiskFreeAction->setEnabled(false);
    }
}

SpaceInfoToolsMenu::~SpaceInfoToolsMenu()
{
}


