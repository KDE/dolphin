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

#include "navigationsettingspage.h"

#include "settings/dolphinsettings.h"

#include "dolphin_generalsettings.h"

#include <kdialog.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kvbox.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

NavigationSettingsPage::NavigationSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_browseThroughArchives(0),
    m_autoExpandFolders(0)
{
    const int spacing = KDialog::spacingHint();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(spacing);

    // create 'Mouse' group
    QGroupBox* mouseBox = new QGroupBox(i18nc("@title:group", "Mouse"), vBox);
    m_singleClick = new QRadioButton(i18nc("@option:check Mouse Settings",
                                           "Single-click to open files and folders"), mouseBox);
    connect(m_singleClick, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    m_doubleClick = new QRadioButton(i18nc("@option:check Mouse Settings",
                                           "Double-click to open files and folders"), mouseBox);
    connect(m_doubleClick, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    QVBoxLayout* mouseBoxLayout = new QVBoxLayout(mouseBox);
    mouseBoxLayout->addWidget(m_singleClick);
    mouseBoxLayout->addWidget(m_doubleClick);

    m_browseThroughArchives = new QCheckBox(i18nc("@option:check", "Browse through archives"), vBox);
    connect(m_browseThroughArchives, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    m_autoExpandFolders = new QCheckBox(i18nc("option:check", "Open folders during drag operations"), vBox);
    connect(m_autoExpandFolders, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();
}

NavigationSettingsPage::~NavigationSettingsPage()
{
}

void NavigationSettingsPage::applySettings()
{
    KConfig config("kcminputrc");
    KConfigGroup group = config.group("KDE");
    group.writeEntry("SingleClick", m_singleClick->isChecked(), KConfig::Persistent|KConfig::Global);
    config.sync();
    KGlobalSettings::self()->emitChange(KGlobalSettings::SettingsChanged, KGlobalSettings::SETTINGS_MOUSE);

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->setBrowseThroughArchives(m_browseThroughArchives->isChecked());
    settings->setAutoExpandFolders(m_autoExpandFolders->isChecked());
}

void NavigationSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
    //Default kde setting is to be single click navigation.
    m_singleClick->setChecked(true);
    m_doubleClick->setChecked(false);
}

void NavigationSettingsPage::loadSettings()
{
    const bool singleClick = KGlobalSettings::singleClick();
    m_singleClick->setChecked(singleClick);
    m_doubleClick->setChecked(!singleClick);
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    m_browseThroughArchives->setChecked(settings->browseThroughArchives());
    m_autoExpandFolders->setChecked(settings->autoExpandFolders());
}

#include "navigationsettingspage.moc"
