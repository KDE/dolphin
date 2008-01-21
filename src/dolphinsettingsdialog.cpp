/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#include "dolphinsettingsdialog.h"

#include "dolphinapplication.h"
#include "dolphinmainwindow.h"
#include "generalsettingspage.h"
#include "startupsettingspage.h"
#include "viewsettingspage.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kicon.h>

DolphinSettingsDialog::DolphinSettingsDialog(DolphinMainWindow* mainWindow) :
    KPageDialog(mainWindow),
    m_startupSettingsPage(0),
    m_generalSettingsPage(0),
    m_viewSettingsPage(0)

{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(512, minSize.height()));

    setFaceType(List);
    setCaption(i18nc("@title:window", "Dolphin Preferences"));
    setButtons(Ok | Apply | Cancel | Default);
    setDefaultButton(Ok);

    m_startupSettingsPage = new StartupSettingsPage(mainWindow, this);
    KPageWidgetItem* startupSettingsFrame = addPage(m_startupSettingsPage,
                                                    i18nc("@title:group", "Startup"));
    startupSettingsFrame->setIcon(KIcon("go-home"));

    m_generalSettingsPage = new GeneralSettingsPage(mainWindow, this);
    KPageWidgetItem* generalSettingsFrame = addPage(m_generalSettingsPage,
                                                    i18nc("@title:group", "General"));
    generalSettingsFrame->setIcon(KIcon("system-run"));

    m_viewSettingsPage = new ViewSettingsPage(mainWindow, this);
    KPageWidgetItem* viewSettingsFrame = addPage(m_viewSettingsPage,
                                                 i18nc("@title:group", "View Modes"));
    viewSettingsFrame->setIcon(KIcon("view-choose"));

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"), "SettingsDialog");
    restoreDialogSize(dialogConfig);
}

DolphinSettingsDialog::~DolphinSettingsDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"), "SettingsDialog");
    saveDialogSize(dialogConfig);
}

void DolphinSettingsDialog::slotButtonClicked(int button)
{
    if ((button == Ok) || (button == Apply)) {
        applySettings();
    } else if (button == Default) {
        const QString text(i18nc("@info", "All settings will be reset to default values. Do you want to continue?"));
        if (KMessageBox::questionYesNo(this, text) == KMessageBox::Yes) {
            restoreDefaults();
        }
    }

    KPageDialog::slotButtonClicked(button);
}

void DolphinSettingsDialog::applySettings()
{
    m_startupSettingsPage->applySettings();
    m_generalSettingsPage->applySettings();
    m_viewSettingsPage->applySettings();
    DolphinApplication::app()->refreshMainWindows();
}

void DolphinSettingsDialog::restoreDefaults()
{
    m_startupSettingsPage->restoreDefaults();
    m_generalSettingsPage->restoreDefaults();
    m_viewSettingsPage->restoreDefaults();
    DolphinApplication::app()->refreshMainWindows();
}

#include "dolphinsettingsdialog.moc"
