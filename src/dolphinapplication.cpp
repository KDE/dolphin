/***************************************************************************
 *   Copyright (C) 2006-2011 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2006 by Holger 'zecke' Freyther <freyther@kde.org>      *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "dolphinapplication.h"
#include "dolphinmainwindow.h"
#include "dolphin_generalsettings.h"
#include "settings/dolphinsettings.h"

#include <KCmdLineArgs>
#include <KDebug>
#include <KRun>
#include <KUrl>

DolphinApplication::DolphinApplication() :
    m_mainWindow(0)
{
    m_mainWindow = new DolphinMainWindow();
    m_mainWindow->setAttribute(Qt::WA_DeleteOnClose);
    m_mainWindow->show();

    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    bool resetSplitSettings = false;
    GeneralSettings* generalSettings = DolphinSettings::instance().generalSettings();
    if (args->isSet("split") && !generalSettings->splitView()) {
        // Dolphin should be opened with a split view although this is not
        // set in the GeneralSettings. Temporary adjust the setting until
        // all passed URLs have been opened.
        generalSettings->setSplitView(true);
        resetSplitSettings = true;
    }

    const int argsCount = args->count();
    if (argsCount > 0) {
        QList<KUrl> urls;
        for (int i = 0; i < argsCount; ++i) {
            const KUrl url = args->url(i);
            if (url.isValid()) {
                urls.append(url);
            }
        }

        if (!urls.isEmpty()) {
            if (args->isSet("select")) {
                m_mainWindow->openFiles(urls);
            } else {
                m_mainWindow->openDirectories(urls);
            }
        }
    }
    args->clear();

    if (resetSplitSettings) {
        generalSettings->setSplitView(false);
    }
}

DolphinApplication::~DolphinApplication()
{
}

DolphinApplication* DolphinApplication::app()
{
    return qobject_cast<DolphinApplication*>(qApp);
}

void DolphinApplication::restoreSession()
{
    const QString className = KXmlGuiWindow::classNameOfToplevel(1);
    if (className == QLatin1String("DolphinMainWindow")) {
        m_mainWindow->restore(1);
    } else {
        kWarning() << "Unknown class " << className << " in session saved data!";
    }
}

#include "dolphinapplication.moc"
