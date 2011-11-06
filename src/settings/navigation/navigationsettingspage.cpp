/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "dolphin_generalsettings.h"

#include <KDialog>
#include <KGlobalSettings>
#include <KLocale>
#include <KVBox>

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

NavigationSettingsPage::NavigationSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_openArchivesAsFolder(0),
    m_autoExpandFolders(0)
{
    const int spacing = KDialog::spacingHint();

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    KVBox* vBox = new KVBox(this);
    vBox->setSpacing(spacing);

    // create 'Mouse' group
    QGroupBox* mouseBox = new QGroupBox(i18nc("@title:group", "Mouse"), vBox);
    mouseBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_singleClick = new QRadioButton(i18nc("@option:check Mouse Settings",
                                           "Single-click to open files and folders"), mouseBox);
    m_doubleClick = new QRadioButton(i18nc("@option:check Mouse Settings",
                                           "Double-click to open files and folders"), mouseBox);

    QVBoxLayout* mouseBoxLayout = new QVBoxLayout(mouseBox);
    mouseBoxLayout->addWidget(m_singleClick);
    mouseBoxLayout->addWidget(m_doubleClick);

    m_openArchivesAsFolder = new QCheckBox(i18nc("@option:check", "Open archives as folder"), vBox);

    m_autoExpandFolders = new QCheckBox(i18nc("option:check", "Open folders during drag operations"), vBox);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();

    connect(m_singleClick, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_doubleClick, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_openArchivesAsFolder, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_autoExpandFolders, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
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

    GeneralSettings* settings = GeneralSettings::self();
    settings->setBrowseThroughArchives(m_openArchivesAsFolder->isChecked());
    settings->setAutoExpandFolders(m_autoExpandFolders->isChecked());

    settings->writeConfig();
}

void NavigationSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);

    // The mouse settings stored in KGlobalSettings must be reset to
    // the default values (= single click) manually.
    m_singleClick->setChecked(true);
    m_doubleClick->setChecked(false);
}

void NavigationSettingsPage::loadSettings()
{
    const bool singleClick = KGlobalSettings::singleClick();
    m_singleClick->setChecked(singleClick);
    m_doubleClick->setChecked(!singleClick);
    m_openArchivesAsFolder->setChecked(GeneralSettings::browseThroughArchives());
    m_autoExpandFolders->setChecked(GeneralSettings::autoExpandFolders());
}

#include "navigationsettingspage.moc"
