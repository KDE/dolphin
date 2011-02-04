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
#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"
#include "general/generalsettingspage.h"
#include "navigation/navigationsettingspage.h"
#include "services/servicessettingspage.h"
#include "startup/startupsettingspage.h"
#include "viewmodes/viewsettingspage.h"
#include "trash/trashsettingspage.h"

#include <KLocale>
#include <KMessageBox>
#include <KIcon>

DolphinSettingsDialog::DolphinSettingsDialog(const KUrl& url, QWidget* parent) :
    KPageDialog(parent),
    m_pages()

{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(512, minSize.height()));

    setFaceType(List);
    setCaption(i18nc("@title:window", "Dolphin Preferences"));
    setButtons(Ok | Apply | Cancel | Default);
    enableButtonApply(false);
    setDefaultButton(Ok);

    // Startup
    StartupSettingsPage* startupSettingsPage = new StartupSettingsPage(url, this);
    KPageWidgetItem* startupSettingsFrame = addPage(startupSettingsPage,
                                                    i18nc("@title:group", "Startup"));
    startupSettingsFrame->setIcon(KIcon("go-home"));
    connect(startupSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    // View Modes
    ViewSettingsPage* viewSettingsPage = new ViewSettingsPage(this);
    KPageWidgetItem* viewSettingsFrame = addPage(viewSettingsPage,
                                                 i18nc("@title:group", "View Modes"));
    viewSettingsFrame->setIcon(KIcon("view-choose"));
    connect(viewSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    // Navigation
    NavigationSettingsPage* navigationSettingsPage = new NavigationSettingsPage(this);
    KPageWidgetItem* navigationSettingsFrame = addPage(navigationSettingsPage,
                                                       i18nc("@title:group", "Navigation"));
    navigationSettingsFrame->setIcon(KIcon("input-mouse"));
    connect(navigationSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    // Services
    ServicesSettingsPage* servicesSettingsPage = new ServicesSettingsPage(this);
    KPageWidgetItem* servicesSettingsFrame = addPage(servicesSettingsPage,
                                                       i18nc("@title:group", "Services"));
    servicesSettingsFrame->setIcon(KIcon("services"));
    connect(servicesSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    // Trash
    TrashSettingsPage* trashSettingsPage = new TrashSettingsPage(this);
    KPageWidgetItem* trashSettingsFrame = addPage(trashSettingsPage,
                                                   i18nc("@title:group", "Trash"));
    trashSettingsFrame->setIcon(KIcon("user-trash"));
    connect(trashSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    // General
    GeneralSettingsPage* generalSettingsPage = new GeneralSettingsPage(url, this);
    KPageWidgetItem* generalSettingsFrame = addPage(generalSettingsPage,
                                                    i18nc("@title:group General settings", "General"));
    generalSettingsFrame->setIcon(KIcon("system-run"));
    connect(generalSettingsPage, SIGNAL(changed()), this, SLOT(enableApply()));

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"), "SettingsDialog");
    restoreDialogSize(dialogConfig);

    m_pages.append(startupSettingsPage);
    m_pages.append(viewSettingsPage);
    m_pages.append(navigationSettingsPage);
    m_pages.append(servicesSettingsPage);
    m_pages.append(trashSettingsPage);
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
        restoreDefaults();
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

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    if (settings->modifiedStartupSettings()) {
        // Reset the modified startup settings hint. The changed startup settings
        // have been applied already in app()->refreshMainWindows().
        settings->setModifiedStartupSettings(false);
        settings->writeConfig();
    }

    enableButtonApply(false);
}

void DolphinSettingsDialog::restoreDefaults()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }
}

#include "dolphinsettingsdialog.moc"
