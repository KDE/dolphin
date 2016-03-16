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

#include <dolphinmainwindow.h>
#include "dolphin_generalsettings.h"
#include "general/generalsettingspage.h"
#include "navigation/navigationsettingspage.h"
#include "services/servicessettingspage.h"
#include "startup/startupsettingspage.h"
#include "viewmodes/viewsettingspage.h"
#include "trash/trashsettingspage.h"

#include <KWindowConfig>
#include <KLocalizedString>
#include <QIcon>

#include <QPushButton>
#include <QDialogButtonBox>

DolphinSettingsDialog::DolphinSettingsDialog(const QUrl& url, QWidget* parent) :
    KPageDialog(parent),
    m_pages()

{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(512, minSize.height()));

    setFaceType(List);
    setWindowTitle(i18nc("@title:window", "Dolphin Preferences"));
    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Ok
            | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults);
    box->button(QDialogButtonBox::Apply)->setEnabled(false);
    box->button(QDialogButtonBox::Ok)->setDefault(true);
    setButtonBox(box);

    connect(box->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, &DolphinSettingsDialog::applySettings);
    connect(box->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &DolphinSettingsDialog::applySettings);
    connect(box->button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, this, &DolphinSettingsDialog::restoreDefaults);

    // Startup
    StartupSettingsPage* startupSettingsPage = new StartupSettingsPage(url, this);
    KPageWidgetItem* startupSettingsFrame = addPage(startupSettingsPage,
                                                    i18nc("@title:group", "Startup"));
    startupSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("go-home")));
    connect(startupSettingsPage, &StartupSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // View Modes
    ViewSettingsPage* viewSettingsPage = new ViewSettingsPage(this);
    KPageWidgetItem* viewSettingsFrame = addPage(viewSettingsPage,
                                                 i18nc("@title:group", "View Modes"));
    viewSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("view-choose")));
    connect(viewSettingsPage, &ViewSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Navigation
    NavigationSettingsPage* navigationSettingsPage = new NavigationSettingsPage(this);
    KPageWidgetItem* navigationSettingsFrame = addPage(navigationSettingsPage,
                                                       i18nc("@title:group", "Navigation"));
    navigationSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("edit-select")));
    connect(navigationSettingsPage, &NavigationSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Services
    ServicesSettingsPage* servicesSettingsPage = new ServicesSettingsPage(this);
    KPageWidgetItem* servicesSettingsFrame = addPage(servicesSettingsPage,
                                                       i18nc("@title:group", "Services"));
    servicesSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("flag")));
    connect(servicesSettingsPage, &ServicesSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // Trash
    TrashSettingsPage* trashSettingsPage = new TrashSettingsPage(this);
    KPageWidgetItem* trashSettingsFrame = addPage(trashSettingsPage,
                                                   i18nc("@title:group", "Trash"));
    trashSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("trash-empty")));
    connect(trashSettingsPage, &TrashSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    // General
    GeneralSettingsPage* generalSettingsPage = new GeneralSettingsPage(url, this);
    KPageWidgetItem* generalSettingsFrame = addPage(generalSettingsPage,
                                                    i18nc("@title:group General settings", "General"));
    generalSettingsFrame->setIcon(QIcon::fromTheme(QStringLiteral("view-preview")));
    connect(generalSettingsPage, &GeneralSettingsPage::changed, this, &DolphinSettingsDialog::enableApply);

    m_pages.append(startupSettingsPage);
    m_pages.append(viewSettingsPage);
    m_pages.append(navigationSettingsPage);
    m_pages.append(servicesSettingsPage);
    m_pages.append(trashSettingsPage);
    m_pages.append(generalSettingsPage);

    const KConfigGroup dialogConfig(KSharedConfig::openConfig(QStringLiteral("dolphinrc")), "SettingsDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), dialogConfig);
}

DolphinSettingsDialog::~DolphinSettingsDialog()
{
    KConfigGroup dialogConfig(KSharedConfig::openConfig(QStringLiteral("dolphinrc")), "SettingsDialog");
    KWindowConfig::saveWindowSize(windowHandle(), dialogConfig);
}

void DolphinSettingsDialog::enableApply()
{
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void DolphinSettingsDialog::applySettings()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->applySettings();
    }

    emit settingsChanged();

    GeneralSettings* settings = GeneralSettings::self();
    if (settings->modifiedStartupSettings()) {
        // Reset the modified startup settings hint. The changed startup settings
        // have been applied already due to emitting settingsChanged().
        settings->setModifiedStartupSettings(false);
        settings->save();
    }
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void DolphinSettingsDialog::restoreDefaults()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }
}

