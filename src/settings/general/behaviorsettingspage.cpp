/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "behaviorsettingspage.h"

#include "global.h"
#include "views/viewproperties.h"

#include <KLocalizedString>

#include <QButtonGroup>
#include <QCheckBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QSpacerItem>

BehaviorSettingsPage::BehaviorSettingsPage(const QUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_url(url),
    m_localViewProps(nullptr),
    m_globalViewProps(nullptr),
    m_showToolTips(nullptr),
    m_showSelectionToggle(nullptr),
    m_naturalSorting(nullptr),
    m_caseSensitiveSorting(nullptr),
    m_caseInsensitiveSorting(nullptr),
    m_renameInline(nullptr),
    m_useTabForSplitViewSwitch(nullptr)
{
    QFormLayout* topLayout = new QFormLayout(this);


    // View properties
    m_globalViewProps = new QRadioButton(i18nc("@option:radio", "Use common display style for all folders"));
    m_localViewProps = new QRadioButton(i18nc("@option:radio", "Remember display style for each folder"));
    m_localViewProps->setToolTip(i18nc("@info", "Dolphin will create a hidden .directory file in each folder you change view properties for."));

    QButtonGroup* viewGroup = new QButtonGroup(this);
    viewGroup->addButton(m_globalViewProps);
    viewGroup->addButton(m_localViewProps);
    topLayout->addRow(i18nc("@title:group", "View: "), m_globalViewProps);
    topLayout->addRow(QString(), m_localViewProps);


    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));


    // Sorting properties
    m_naturalSorting = new QRadioButton(i18nc("option:radio", "Natural"));
    m_caseInsensitiveSorting = new QRadioButton(i18nc("option:radio", "Alphabetical, case insensitive"));
    m_caseSensitiveSorting = new QRadioButton(i18nc("option:radio", "Alphabetical, case sensitive"));

    QButtonGroup* sortingModeGroup = new QButtonGroup(this);
    sortingModeGroup->addButton(m_naturalSorting);
    sortingModeGroup->addButton(m_caseInsensitiveSorting);
    sortingModeGroup->addButton(m_caseSensitiveSorting);
    topLayout->addRow(i18nc("@title:group", "Sorting mode: "), m_naturalSorting);
    topLayout->addRow(QString(), m_caseInsensitiveSorting);
    topLayout->addRow(QString(), m_caseSensitiveSorting);


    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));


#ifdef HAVE_BALOO
    // 'Show tooltips'
    m_showToolTips = new QCheckBox(i18nc("@option:check", "Show tooltips"));
    topLayout->addRow(i18nc("@title:group", "Miscellaneous: "), m_showToolTips);
#endif

    // 'Show selection marker'
    m_showSelectionToggle = new QCheckBox(i18nc("@option:check", "Show selection marker"));
#ifdef HAVE_BALOO
    topLayout->addRow(QString(), m_showSelectionToggle);
#else
    topLayout->addRow(i18nc("@title:group", "Miscellaneous: "), m_showSelectionToggle);
#endif

    // 'Inline renaming of items'
    m_renameInline = new QCheckBox(i18nc("option:check", "Rename inline"));
    topLayout->addRow(QString(), m_renameInline);

    // 'Switch between panes of split views with tab key'
    m_useTabForSplitViewSwitch = new QCheckBox(i18nc("option:check", "Switch between split views panes with tab key"));
    topLayout->addRow(QString(), m_useTabForSplitViewSwitch);

    // 'Close active pane when turning off split view'
    m_closeActiveSplitView = new QCheckBox(i18nc("option:check", "Turning off split view closes active pane"));
    topLayout->addRow(QString(), m_closeActiveSplitView);
    m_closeActiveSplitView->setToolTip(i18n("When deactivated, turning off split view will close the inactive pane"));

    loadSettings();

    connect(m_localViewProps, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_globalViewProps, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
#ifdef HAVE_BALOO
    connect(m_showToolTips, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
#endif
    connect(m_showSelectionToggle, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_naturalSorting, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_caseInsensitiveSorting, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_caseSensitiveSorting, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_renameInline, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_useTabForSplitViewSwitch, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_closeActiveSplitView, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
}

BehaviorSettingsPage::~BehaviorSettingsPage()
{
}

void BehaviorSettingsPage::applySettings()
{
    GeneralSettings* settings = GeneralSettings::self();
    ViewProperties props(m_url);  // read current view properties

    const bool useGlobalViewProps = m_globalViewProps->isChecked();
    settings->setGlobalViewProps(useGlobalViewProps);
#ifdef HAVE_BALOO
    settings->setShowToolTips(m_showToolTips->isChecked());
#endif
    settings->setShowSelectionToggle(m_showSelectionToggle->isChecked());
    setSortingChoiceValue(settings);
    settings->setRenameInline(m_renameInline->isChecked());
    settings->setUseTabForSwitchingSplitView(m_useTabForSplitViewSwitch->isChecked());
    settings->setCloseActiveSplitView(m_closeActiveSplitView->isChecked());
    settings->save();

    if (useGlobalViewProps) {
        // Remember the global view properties by applying the current view properties.
        // It is important that GeneralSettings::globalViewProps() is set before
        // the class ViewProperties is used, as ViewProperties uses this setting
        // to find the destination folder for storing the view properties.
        ViewProperties globalProps(m_url);
        globalProps.setDirProperties(props);
    }
}

void BehaviorSettingsPage::restoreDefaults()
{
    GeneralSettings* settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void BehaviorSettingsPage::loadSettings()
{
    const bool useGlobalViewProps = GeneralSettings::globalViewProps();
    m_localViewProps->setChecked(!useGlobalViewProps);
    m_globalViewProps->setChecked(useGlobalViewProps);

#ifdef HAVE_BALOO
    m_showToolTips->setChecked(GeneralSettings::showToolTips());
#endif
    m_showSelectionToggle->setChecked(GeneralSettings::showSelectionToggle());
    m_renameInline->setChecked(GeneralSettings::renameInline());
    m_useTabForSplitViewSwitch->setChecked(GeneralSettings::useTabForSwitchingSplitView());
    m_closeActiveSplitView->setChecked(GeneralSettings::closeActiveSplitView());

    loadSortingChoiceSettings();
}

void BehaviorSettingsPage::setSortingChoiceValue(GeneralSettings *settings)
{
    using Choice = GeneralSettings::EnumSortingChoice;
    if (m_naturalSorting->isChecked()) {
        settings->setSortingChoice(Choice::NaturalSorting);
    } else if (m_caseInsensitiveSorting->isChecked()) {
        settings->setSortingChoice(Choice::CaseInsensitiveSorting);
    } else if (m_caseSensitiveSorting->isChecked()) {
        settings->setSortingChoice(Choice::CaseSensitiveSorting);
    }
}

void BehaviorSettingsPage::loadSortingChoiceSettings()
{
    using Choice = GeneralSettings::EnumSortingChoice;
    switch (GeneralSettings::sortingChoice()) {
    case Choice::NaturalSorting:
        m_naturalSorting->setChecked(true);
        break;
    case Choice::CaseInsensitiveSorting:
        m_caseInsensitiveSorting->setChecked(true);
        break;
    case Choice::CaseSensitiveSorting:
        m_caseSensitiveSorting->setChecked(true);
        break;
    default:
        Q_UNREACHABLE();
    }
}
