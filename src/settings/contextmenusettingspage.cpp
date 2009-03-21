/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "contextmenusettingspage.h"
#include "dolphinsettings.h"
#include "dolphin_generalsettings.h"

#include <kdialog.h>
#include <klocale.h>
#include <kvbox.h>

#include <QCheckBox>
#include <QVBoxLayout>

const bool SHOW_DELETE = false;

ContextMenuSettingsPage::ContextMenuSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_showDeleteCommand(0),
    m_showCopyMoveMenu(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(KDialog::spacingHint());

    m_showDeleteCommand = new QCheckBox(i18nc("@option:check", "Show 'Delete' command"), vBox);
    connect(m_showDeleteCommand, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    m_showCopyMoveMenu = new QCheckBox(i18nc("@option:check", "Show 'Copy To' and 'Move To' commands"), vBox);
    connect(m_showCopyMoveMenu, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();
}

ContextMenuSettingsPage::~ContextMenuSettingsPage()
{
}

void ContextMenuSettingsPage::applySettings()
{
    KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
    KConfigGroup configGroup(globalConfig, "KDE");
    configGroup.writeEntry("ShowDeleteCommand", m_showDeleteCommand->isChecked());
    configGroup.sync();

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->setShowCopyMoveMenu(m_showCopyMoveMenu->isChecked());
    settings->writeConfig();
}

void ContextMenuSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
    m_showDeleteCommand->setChecked(SHOW_DELETE);
}

void ContextMenuSettingsPage::loadSettings()
{
    KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::IncludeGlobals);
    KConfigGroup configGroup(globalConfig, "KDE");
    m_showDeleteCommand->setChecked(configGroup.readEntry("ShowDeleteCommand", SHOW_DELETE));

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_showCopyMoveMenu->setChecked(settings->showCopyMoveMenu());
}

#include "contextmenusettingspage.moc"
