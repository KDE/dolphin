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

#include <QButtonGroup>
#include <QRadioButton>
#include <QSpacerItem>
#include <QStyle>
#include <QStyleOption>

StatusAndLocationBarsSettingsPage::StatusAndLocationBarsSettingsPage(QWidget *parent, FoldersTabsSettingsPage *foldersPage)
    : SettingsPageBase(parent)
    , m_editableUrl(nullptr)
    , m_showFullPath(nullptr)
    , m_statusBarButtonGroup(nullptr)
    , m_showStatusBarSmall(nullptr)
    , m_showStatusBarFullWidth(nullptr)
    , m_showZoomSlider(nullptr)
    , m_disableStatusBar(nullptr)
{
    // We need to update some urls at the Folders & Tabs tab. We get that from foldersPage and set it on a private attribute
    // foldersTabsPage. That way, we can modify the necessary stuff from here. Specifically, any changes on locationUpdateInitialViewOptions()
    // which is a copy of updateInitialViewOptions() on Folders & Tabs.
    foldersTabsPage = foldersPage;

    QFormLayout *topLayout = new QFormLayout(this);

    // Status bar
    m_statusBarButtonGroup = new QButtonGroup(this);
    m_showStatusBarSmall = new QRadioButton(i18nc("@option:radio", "Small"), this);
    m_showStatusBarFullWidth = new QRadioButton(i18nc("@option:radio", "Full width"), this);
    m_showZoomSlider = new QCheckBox(i18nc("@option:check", "Show zoom slider"), this);
    m_disableStatusBar = new QRadioButton(i18nc("@option:check", "Disabled"), this);

    m_statusBarButtonGroup->addButton(m_showStatusBarSmall, GeneralSettings::EnumShowStatusBar::Small);
    m_statusBarButtonGroup->addButton(m_showStatusBarFullWidth, GeneralSettings::EnumShowStatusBar::FullWidth);
    m_statusBarButtonGroup->addButton(m_disableStatusBar, GeneralSettings::EnumShowStatusBar::Disabled);

    topLayout->addRow(i18nc("@title:group", "Status Bar: "), m_showStatusBarSmall);
    topLayout->addRow(QString(), m_showStatusBarFullWidth);

    // Indent the m_showZoomSlider checkbox under m_showStatusBarFullWidth
    QHBoxLayout *zoomSliderLayout = new QHBoxLayout(this);
    QStyleOption opt;
    opt.initFrom(this);
    zoomSliderLayout->addItem(
        new QSpacerItem(style()->pixelMetric(QStyle::PM_IndicatorWidth, &opt, this), Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));
    zoomSliderLayout->addWidget(m_showZoomSlider);

    topLayout->addRow(QString(), zoomSliderLayout->layout());

    topLayout->addRow(QString(), m_disableStatusBar);
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

    connect(m_statusBarButtonGroup, &QButtonGroup::idClicked, this, &StatusAndLocationBarsSettingsPage::changed);
    connect(m_statusBarButtonGroup, &QButtonGroup::idClicked, this, &StatusAndLocationBarsSettingsPage::onShowStatusBarToggled);
    connect(m_showZoomSlider, &QCheckBox::toggled, this, &StatusAndLocationBarsSettingsPage::changed);
}

StatusAndLocationBarsSettingsPage::~StatusAndLocationBarsSettingsPage()
{
}

void StatusAndLocationBarsSettingsPage::applySettings()
{
    GeneralSettings *settings = GeneralSettings::self();

    settings->setEditableUrl(m_editableUrl->isChecked());
    settings->setShowFullPath(m_showFullPath->isChecked());

    settings->setShowStatusBar(m_statusBarButtonGroup->checkedId());
    settings->setShowZoomSlider(m_showZoomSlider->isChecked());

    settings->save();
}

void StatusAndLocationBarsSettingsPage::onShowStatusBarToggled()
{
    const bool checked = (m_statusBarButtonGroup->checkedId() == GeneralSettings::EnumShowStatusBar::FullWidth);
    m_showZoomSlider->setEnabled(checked);
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
    m_statusBarButtonGroup->button(GeneralSettings::showStatusBar())->setChecked(true);
    m_showZoomSlider->setChecked(GeneralSettings::showZoomSlider());

    onShowStatusBarToggled();
}

#include "moc_statusandlocationbarssettingspage.cpp"
