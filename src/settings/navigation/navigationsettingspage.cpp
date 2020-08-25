/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "navigationsettingspage.h"

#include "dolphin_generalsettings.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QVBoxLayout>

NavigationSettingsPage::NavigationSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_openArchivesAsFolder(nullptr),
    m_autoExpandFolders(nullptr)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    QWidget* vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout(vBox);
    vBoxLayout->setContentsMargins(0, 0, 0, 0);
    vBoxLayout->setAlignment(Qt::AlignTop);

    m_openArchivesAsFolder = new QCheckBox(i18nc("@option:check", "Open archives as folder"), vBox);
    vBoxLayout->addWidget(m_openArchivesAsFolder);

    m_autoExpandFolders = new QCheckBox(i18nc("option:check", "Open folders during drag operations"), vBox);
    vBoxLayout->addWidget(m_autoExpandFolders);

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

