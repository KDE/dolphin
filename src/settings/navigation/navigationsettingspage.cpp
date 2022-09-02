/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "navigationsettingspage.h"

#include "global.h"
#include "dolphin_generalsettings.h"

#include <KLocalizedString>

#include <QButtonGroup>
#include <QCheckBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QVBoxLayout>

NavigationSettingsPage::NavigationSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_openFolderInNewTab(nullptr),
    m_openFolderInNewWindow(nullptr),
    m_openArchivesAsFolder(nullptr),
    m_autoExpandFolders(nullptr),
    m_openNewTabAfterLastTab(nullptr),
    m_openNewTabAfterCurrentTab(nullptr)
{
    QFormLayout* topLayout = new QFormLayout(this);

    // External call/middle click behavior
    m_openFolderInNewTab = new QRadioButton(i18nc("@option:radio", "In new tabs"));
    m_openFolderInNewWindow = new QRadioButton(i18nc("@option:radio", "In new windows"));

    QButtonGroup* newFolderGroup = new QButtonGroup(this);
    newFolderGroup->addButton(m_openFolderInNewTab);
    newFolderGroup->addButton(m_openFolderInNewWindow);

    topLayout->addRow(i18nc("@title:group", "Open new folders: "), m_openFolderInNewTab);
    topLayout->addRow(QString(), m_openFolderInNewWindow);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Tabs properties
    m_openNewTabAfterCurrentTab = new QRadioButton(i18nc("option:radio", "After current tab"));
    m_openNewTabAfterLastTab = new QRadioButton(i18nc("option:radio", "At end of tab bar"));
    QButtonGroup* tabsBehaviorGroup = new QButtonGroup(this);
    tabsBehaviorGroup->addButton(m_openNewTabAfterCurrentTab);
    tabsBehaviorGroup->addButton(m_openNewTabAfterLastTab);
    topLayout->addRow(i18nc("@title:group", "Open new tabs: "), m_openNewTabAfterCurrentTab);
    topLayout->addRow(QString(), m_openNewTabAfterLastTab);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    m_openArchivesAsFolder = new QCheckBox(i18nc("@option:check", "Open archives as folder"));
    m_autoExpandFolders = new QCheckBox(i18nc("option:check", "Open folders during drag operations"));
    topLayout->addRow(i18nc("@title:group", "General: "), m_openArchivesAsFolder);
    topLayout->addRow(QString(), m_autoExpandFolders);

    loadSettings();

    connect(m_openFolderInNewTab, &QRadioButton::toggled, this, &NavigationSettingsPage::changed);
    connect(m_openFolderInNewWindow, &QRadioButton::toggled, this, &NavigationSettingsPage::changed);
    connect(m_openNewTabAfterCurrentTab, &QRadioButton::toggled, this, &NavigationSettingsPage::changed);
    connect(m_openNewTabAfterLastTab, &QRadioButton::toggled, this, &NavigationSettingsPage::changed);
    connect(m_openArchivesAsFolder, &QCheckBox::toggled, this, &NavigationSettingsPage::changed);
    connect(m_autoExpandFolders, &QCheckBox::toggled, this, &NavigationSettingsPage::changed);
}

NavigationSettingsPage::~NavigationSettingsPage()
{
}

void NavigationSettingsPage::applySettings()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->setOpenFolderInNewTab(m_openFolderInNewTab->isChecked());    
    settings->setOpenNewTabAfterLastTab(m_openNewTabAfterLastTab->isChecked());
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
    m_openFolderInNewTab->setChecked(GeneralSettings::openFolderInNewTab());
    m_openFolderInNewWindow->setChecked(!m_openFolderInNewTab->isChecked());
    m_openArchivesAsFolder->setChecked(GeneralSettings::browseThroughArchives());
    m_autoExpandFolders->setChecked(GeneralSettings::autoExpandFolders());
    m_openNewTabAfterLastTab->setChecked(GeneralSettings::openNewTabAfterLastTab());
    m_openNewTabAfterCurrentTab->setChecked(!m_openNewTabAfterLastTab->isChecked());
}

