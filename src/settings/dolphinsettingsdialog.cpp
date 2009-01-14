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

#include <dolphinapplication.h>
#include <dolphinmainwindow.h>
#include "generalsettingspage.h"
#include "startupsettingspage.h"
#include "viewsettingspage.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kicon.h>

DolphinSettingsDialog::DolphinSettingsDialog(DolphinMainWindow* mainWindow) :
    KPageDialog(mainWindow),
    m_pages()

{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(512, minSize.height()));

    setFaceType(List);
    setCaption(i18nc("@title:window", "Dolphin Preferences"));
    setButtons(Ok | Apply | Cancel | Default);
    enableButtonApply(false);
    setDefaultButton(Ok);

    StartupSettingsPage* startupSettingsPage = new StartupSettingsPage(mainWindow, this);
    KPageWidgetItem* startupSettingsFrame = addPage(startupSettingsPage,
                                                    i18nc("@title:group", "Startup"));
    startupSettingsFrame->setIcon(KIcon("go-home"));
    connect(startupSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    ViewSettingsPage* viewSettingsPage = new ViewSettingsPage(mainWindow, this);
    KPageWidgetItem* viewSettingsFrame = addPage(viewSettingsPage,
                                                 i18nc("@title:group", "View Modes"));
    viewSettingsFrame->setIcon(KIcon("view-choose"));
    connect(viewSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    GeneralSettingsPage* generalSettingsPage = new GeneralSettingsPage(mainWindow, this);
    KPageWidgetItem* generalSettingsFrame = addPage(generalSettingsPage,
                                                    i18nc("@title:group General settings", "General"));
    generalSettingsFrame->setIcon(KIcon("system-run"));
    connect(generalSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"), "SettingsDialog");
    restoreDialogSize(dialogConfig);

    m_pages.append(startupSettingsPage);
    m_pages.append(viewSettingsPage);
    m_pages.append(generalSettingsPage);
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

void DolphinSettingsDialog::enableApply()
{
    enableButtonApply(true);
}

void DolphinSettingsDialog::applySettings()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->applySettings();
    }
    DolphinApplication::app()->refreshMainWindows();
}

void DolphinSettingsDialog::restoreDefaults()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }
    DolphinApplication::app()->refreshMainWindows();
}

#include "dolphinsettingsdialog.moc"
