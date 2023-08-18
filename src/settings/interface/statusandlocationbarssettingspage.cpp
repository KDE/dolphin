/*
 * SPDX-FileCopyrightText: 2023 Dimosthenis Krallis <dimosthenis.krallis@outlook.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "statusandlocationbarssettingspage.h"
#include "dolphinmainwindow.h"
#include "dolphinviewcontainer.h"
#include "settings/interface/folderstabssettingspage.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QFormLayout>

#include <QRadioButton>
#include <QSpacerItem>

StatusAndLocationBarsSettingsPage::StatusAndLocationBarsSettingsPage(QWidget *parent, FoldersTabsSettingsPage *foldersPage)
    : SettingsPageBase(parent)
    , m_editableUrl(nullptr)
    , m_showFullPath(nullptr)
    , m_showStatusBar(nullptr)
    , m_showZoomSlider(nullptr)
    , m_showSpaceInfo(nullptr)
{
    // We need to update some urls at the Folders & Tabs tab. We get that from foldersPage and set it on a private attribute
    // foldersTabsPage. That way, we can modify the necessary stuff from here. Specifically, any changes on locationUpdateInitialViewOptions()
    // which is a copy of updateInitialViewOptions() on Folders & Tabs.
    foldersTabsPage = foldersPage;

    QFormLayout *topLayout = new QFormLayout(this);

    // Status bar
    m_showStatusBar = new QCheckBox(i18nc("@option:check", "Show status bar"), this);
    m_showZoomSlider = new QCheckBox(i18nc("@option:check", "Show zoom slider"), this);
    m_showSpaceInfo = new QCheckBox(i18nc("@option:check", "Show space information"), this);

    topLayout->addRow(i18nc("@title:group", "Status Bar: "), m_showStatusBar);
    topLayout->addRow(QString(), m_showZoomSlider);
    topLayout->addRow(QString(), m_showSpaceInfo);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Location bar
    m_editableUrl = new QCheckBox(i18nc("@option:check Startup Settings", "Make location bar editable"));
    topLayout->addRow(i18n("Location bar:"), m_editableUrl);

    m_showFullPath = new QCheckBox(i18nc("@option:check Startup Settings", "Show full path inside location bar"));
    topLayout->addRow(QString(), m_showFullPath);

    loadSettings();

    locationUpdateInitialViewOptions();

    connect(m_editableUrl, &QCheckBox::toggled, this, &StatusAndLocationBarsSettingsPage::locationSlotSettingsChanged);
    connect(m_showFullPath, &QCheckBox::toggled, this, &StatusAndLocationBarsSettingsPage::locationSlotSettingsChanged);

    connect(m_showStatusBar, &QCheckBox::toggled, this, &StatusAndLocationBarsSettingsPage::changed);
    connect(m_showStatusBar, &QCheckBox::toggled, this, &StatusAndLocationBarsSettingsPage::onShowStatusBarToggled);
    connect(m_showZoomSlider, &QCheckBox::toggled, this, &StatusAndLocationBarsSettingsPage::changed);
    connect(m_showSpaceInfo, &QCheckBox::toggled, this, &StatusAndLocationBarsSettingsPage::changed);
}

StatusAndLocationBarsSettingsPage::~StatusAndLocationBarsSettingsPage()
{
}

void StatusAndLocationBarsSettingsPage::applySettings()
{
    GeneralSettings *settings = GeneralSettings::self();

    settings->setEditableUrl(m_editableUrl->isChecked());
    settings->setShowFullPath(m_showFullPath->isChecked());

    settings->setShowStatusBar(m_showStatusBar->isChecked());
    settings->setShowZoomSlider(m_showZoomSlider->isChecked());
    settings->setShowSpaceInfo(m_showSpaceInfo->isChecked());

    settings->save();
}

void StatusAndLocationBarsSettingsPage::onShowStatusBarToggled()
{
    const bool checked = m_showStatusBar->isChecked();
    m_showZoomSlider->setEnabled(checked);
    m_showSpaceInfo->setEnabled(checked);
}

void StatusAndLocationBarsSettingsPage::restoreDefaults()
{
    GeneralSettings *settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void StatusAndLocationBarsSettingsPage::locationSlotSettingsChanged()
{
    // Provide a hint that the startup settings have been changed. This allows the views
    // to apply the startup settings only if they have been explicitly changed by the user
    // (see bug #254947).
    GeneralSettings::setModifiedStartupSettings(true);

    // Enable and disable home URL controls appropriately
    locationUpdateInitialViewOptions();
    Q_EMIT changed();
}

void StatusAndLocationBarsSettingsPage::locationUpdateInitialViewOptions()
{
    foldersTabsPage->m_homeUrlBoxLayoutContainer->setEnabled(foldersTabsPage->m_homeUrlRadioButton->isChecked());
    foldersTabsPage->m_buttonBoxLayoutContainer->setEnabled(foldersTabsPage->m_homeUrlRadioButton->isChecked());
}

void StatusAndLocationBarsSettingsPage::loadSettings()
{
    m_editableUrl->setChecked(GeneralSettings::editableUrl());
    m_showFullPath->setChecked(GeneralSettings::showFullPath());
    m_showStatusBar->setChecked(GeneralSettings::showStatusBar());
    m_showZoomSlider->setChecked(GeneralSettings::showZoomSlider());
    m_showSpaceInfo->setChecked(GeneralSettings::showSpaceInfo());

    onShowStatusBarToggled();
}

#include "moc_statusandlocationbarssettingspage.cpp"
