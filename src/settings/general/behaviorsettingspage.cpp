/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
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

#include "behaviorsettingspage.h"

#include "dolphin_generalsettings.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>

#include <views/viewproperties.h>

BehaviorSettingsPage::BehaviorSettingsPage(const QUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_url(url),
    m_localViewProps(0),
    m_globalViewProps(0),
    m_showToolTips(0),
    m_showSelectionToggle(0),
    m_naturalSorting(0),
    m_caseSensitiveSorting(0),
    m_caseInsensitiveSorting(0),
    m_renameInline(0),
    m_useTabForSplitViewSwitch(0)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);

    // View properties
    QGroupBox* viewPropsBox = new QGroupBox(i18nc("@title:group", "View"), this);
    viewPropsBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    m_localViewProps = new QRadioButton(i18nc("@option:radio", "Remember properties for each folder"), viewPropsBox);
    m_globalViewProps = new QRadioButton(i18nc("@option:radio", "Use common properties for all folders"), viewPropsBox);

    QVBoxLayout* viewPropsLayout = new QVBoxLayout(viewPropsBox);
    viewPropsLayout->addWidget(m_localViewProps);
    viewPropsLayout->addWidget(m_globalViewProps);

    // Sorting properties
    QGroupBox* sortingPropsBox = new QGroupBox(i18nc("@title:group", "Sorting Mode"), this);
    sortingPropsBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    m_naturalSorting = new QRadioButton(i18nc("option:radio", "Natural sorting"), sortingPropsBox);
    m_caseInsensitiveSorting = new QRadioButton(i18nc("option:radio", "Alphabetical sorting, case insensitive"), sortingPropsBox);
    m_caseSensitiveSorting = new QRadioButton(i18nc("option:radio", "Alphabetical sorting, case sensitive"), sortingPropsBox);

    QVBoxLayout* sortingPropsLayout = new QVBoxLayout(sortingPropsBox);
    sortingPropsLayout->addWidget(m_naturalSorting);
    sortingPropsLayout->addWidget(m_caseInsensitiveSorting);
    sortingPropsLayout->addWidget(m_caseSensitiveSorting);

    // 'Show tooltips'
    m_showToolTips = new QCheckBox(i18nc("@option:check", "Show tooltips"), this);

    // 'Show selection marker'
    m_showSelectionToggle = new QCheckBox(i18nc("@option:check", "Show selection marker"), this);

    // 'Inline renaming of items'
    m_renameInline = new QCheckBox(i18nc("option:check", "Rename inline"), this);

    // 'Use tab for switching between right and left split'
    m_useTabForSplitViewSwitch = new QCheckBox(i18nc("option:check", "Use tab for switching between right and left split view"), this);

    topLayout->addWidget(viewPropsBox);
    topLayout->addWidget(sortingPropsBox);
    topLayout->addWidget(m_showToolTips);
    topLayout->addWidget(m_showSelectionToggle);
    topLayout->addWidget(m_renameInline);
    topLayout->addWidget(m_useTabForSplitViewSwitch);
    topLayout->addStretch();

    loadSettings();

    connect(m_localViewProps, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_globalViewProps, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_showToolTips, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_showSelectionToggle, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_naturalSorting, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_caseInsensitiveSorting, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_caseSensitiveSorting, &QRadioButton::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_renameInline, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
    connect(m_useTabForSplitViewSwitch, &QCheckBox::toggled, this, &BehaviorSettingsPage::changed);
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
    settings->setShowToolTips(m_showToolTips->isChecked());
    settings->setShowSelectionToggle(m_showSelectionToggle->isChecked());
    setSortingChoiceValue(settings);
    settings->setRenameInline(m_renameInline->isChecked());
    settings->setUseTabForSwitchingSplitView(m_useTabForSplitViewSwitch->isChecked());
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

    m_showToolTips->setChecked(GeneralSettings::showToolTips());
    m_showSelectionToggle->setChecked(GeneralSettings::showSelectionToggle());
    m_renameInline->setChecked(GeneralSettings::renameInline());
    m_useTabForSplitViewSwitch->setChecked(GeneralSettings::useTabForSwitchingSplitView());

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
