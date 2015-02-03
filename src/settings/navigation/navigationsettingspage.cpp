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

#include <KLocalizedString>

#include <QCheckBox>
#include <QVBoxLayout>

NavigationSettingsPage::NavigationSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_openArchivesAsFolder(0),
    m_autoExpandFolders(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    QWidget* vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout(vBox);
    vBoxLayout->setMargin(0);
    vBoxLayout->setAlignment(Qt::AlignTop);

    m_openArchivesAsFolder = new QCheckBox(i18nc("@option:check", "Open archives as folder"), vBox);
    vBoxLayout->addWidget(m_openArchivesAsFolder);

    m_autoExpandFolders = new QCheckBox(i18nc("option:check", "Open folders during drag operations"), vBox);
    vBoxLayout->addWidget(m_autoExpandFolders);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(vBox);

    topLayout->addWidget(vBox);

    loadSettings();

    connect(m_openArchivesAsFolder, &QCheckBox::toggled, this, &NavigationSettingsPage::changed);
    connect(m_autoExpandFolders, &QCheckBox::toggled, this, &NavigationSettingsPage::changed);
}

NavigationSettingsPage::~NavigationSettingsPage()
{
}

void NavigationSettingsPage::applySettings()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->setBrowseThroughArchives(m_openArchivesAsFolder->isChecked());
    settings->setAutoExpandFolders(m_autoExpandFolders->isChecked());

    settings->save();
}

void NavigationSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void NavigationSettingsPage::loadSettings()
{
    m_openArchivesAsFolder->setChecked(GeneralSettings::browseThroughArchives());
    m_autoExpandFolders->setChecked(GeneralSettings::autoExpandFolders());
}

