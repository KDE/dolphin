/*
 * SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "contentdisplaytab.h"
#include "dolphin_contentdisplaysettings.h"
#include "dolphin_generalsettings.h"

#include <KFormat>
#include <KLocalizedString>

#include <QButtonGroup>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QSpinBox>

ContentDisplayTab::ContentDisplayTab(QWidget *parent)
    : SettingsPageBase(parent)
    , m_naturalSorting(nullptr)
    , m_caseSensitiveSorting(nullptr)
    , m_caseInsensitiveSorting(nullptr)
    , m_numberOfItems(nullptr)
    , m_sizeOfContents(nullptr)
    , m_noDirectorySize(nullptr)
    , m_recursiveDirectorySizeLimit(nullptr)
    , m_useRelatetiveDates(nullptr)
    , m_useShortDates(nullptr)
    , m_useSymbolicPermissions(nullptr)
    , m_useNumericPermissions(nullptr)
    , m_useCombinedPermissions(nullptr)
{
    QFormLayout *topLayout = new QFormLayout(this);

    // Sorting Order
    m_naturalSorting = new QRadioButton(i18nc("option:radio", "Natural"));
    m_caseInsensitiveSorting = new QRadioButton(i18nc("option:radio", "Alphabetical, case insensitive"));
    m_caseSensitiveSorting = new QRadioButton(i18nc("option:radio", "Alphabetical, case sensitive"));

    QButtonGroup *sortingOrderGroup = new QButtonGroup(this);
    sortingOrderGroup->addButton(m_naturalSorting);
    sortingOrderGroup->addButton(m_caseInsensitiveSorting);
    sortingOrderGroup->addButton(m_caseSensitiveSorting);
    topLayout->addRow(i18nc("@title:group", "Sorting mode: "), m_naturalSorting);
    topLayout->addRow(QString(), m_caseInsensitiveSorting);
    topLayout->addRow(QString(), m_caseSensitiveSorting);

#ifndef Q_OS_WIN
    // Sorting properties
    m_numberOfItems = new QRadioButton(i18nc("option:radio", "Show number of items"));
    m_sizeOfContents = new QRadioButton(i18nc("option:radio", "Show size of contents, up to "));
    m_noDirectorySize = new QRadioButton(i18nc("option:radio", "Show no size"));

    QButtonGroup *sortingModeGroup = new QButtonGroup(this);
    sortingModeGroup->addButton(m_numberOfItems);
    sortingModeGroup->addButton(m_sizeOfContents);
    sortingModeGroup->addButton(m_noDirectorySize);

    m_recursiveDirectorySizeLimit = new QSpinBox();
    connect(m_recursiveDirectorySizeLimit, &QSpinBox::valueChanged, this, [this](int value) {
        m_recursiveDirectorySizeLimit->setSuffix(i18np(" level deep", " levels deep", value));
    });
    m_recursiveDirectorySizeLimit->setRange(1, 20);
    m_recursiveDirectorySizeLimit->setSingleStep(1);

    QHBoxLayout *contentsSizeLayout = new QHBoxLayout();
    contentsSizeLayout->addWidget(m_sizeOfContents);
    contentsSizeLayout->addWidget(m_recursiveDirectorySizeLimit);

    topLayout->addRow(i18nc("@title:group", "Folder size:"), m_numberOfItems);
    topLayout->addRow(QString(), contentsSizeLayout);
    topLayout->addRow(QString(), m_noDirectorySize);
#endif

    QDateTime thirtyMinutesAgo = QDateTime::currentDateTime().addSecs(-30 * 60);
    QLocale local;
    KFormat formatter(local);

    m_useRelatetiveDates = new QRadioButton(
        i18nc("option:radio as in relative date", "Relative (e.g. '%1')", formatter.formatRelativeDateTime(thirtyMinutesAgo, QLocale::ShortFormat)));
    m_useShortDates =
        new QRadioButton(i18nc("option:radio as in absolute date", "Absolute (e.g. '%1')", local.toString(thirtyMinutesAgo, QLocale::ShortFormat)));

    QButtonGroup *dateFormatGroup = new QButtonGroup(this);
    dateFormatGroup->addButton(m_useRelatetiveDates);
    dateFormatGroup->addButton(m_useShortDates);

    topLayout->addRow(i18nc("@title:group", "Date style:"), m_useRelatetiveDates);
    topLayout->addRow(QString(), m_useShortDates);

    m_useSymbolicPermissions = new QRadioButton(i18nc("option:radio as symbolic style ", "Symbolic (e.g. 'drwxr-xr-x')"));
    m_useNumericPermissions = new QRadioButton(i18nc("option:radio as numeric style", "Numeric (Octal) (e.g. '755')"));
    m_useCombinedPermissions = new QRadioButton(i18nc("option:radio as combined style", "Combined (e.g. 'drwxr-xr-x (755)')"));

    topLayout->addRow(i18nc("@title:group", "Permissions style:"), m_useSymbolicPermissions);
    topLayout->addRow(QString(), m_useNumericPermissions);
    topLayout->addRow(QString(), m_useCombinedPermissions);

    QButtonGroup *permissionsFormatGroup = new QButtonGroup(this);
    permissionsFormatGroup->addButton(m_useSymbolicPermissions);
    permissionsFormatGroup->addButton(m_useNumericPermissions);
    permissionsFormatGroup->addButton(m_useCombinedPermissions);

#ifndef Q_OS_WIN
    connect(m_recursiveDirectorySizeLimit, &QSpinBox::valueChanged, this, &SettingsPageBase::changed);
    connect(m_numberOfItems, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_sizeOfContents, &QRadioButton::toggled, this, [=]() {
        m_recursiveDirectorySizeLimit->setEnabled(m_sizeOfContents->isChecked());
    });
    connect(m_noDirectorySize, &QRadioButton::toggled, this, &SettingsPageBase::changed);
#endif

    connect(m_useRelatetiveDates, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_useShortDates, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_useSymbolicPermissions, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_useNumericPermissions, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_useCombinedPermissions, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_naturalSorting, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_caseInsensitiveSorting, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_caseSensitiveSorting, &QRadioButton::toggled, this, &SettingsPageBase::changed);

    loadSettings();
}

void ContentDisplayTab::applySettings()
{
    auto settings = ContentDisplaySettings::self();
#ifndef Q_OS_WIN
    if (m_numberOfItems->isChecked()) {
        settings->setDirectorySizeMode(ContentDisplaySettings::EnumDirectorySizeMode::ContentCount);
    } else if (m_sizeOfContents->isChecked()) {
        settings->setDirectorySizeMode(ContentDisplaySettings::EnumDirectorySizeMode::ContentSize);
    } else if (m_noDirectorySize->isChecked()) {
        settings->setDirectorySizeMode(ContentDisplaySettings::EnumDirectorySizeMode::None);
    }

    settings->setRecursiveDirectorySizeLimit(m_recursiveDirectorySizeLimit->value());
#endif
    setSortingChoiceValue();
    settings->setUseShortRelativeDates(m_useRelatetiveDates->isChecked());

    if (m_useSymbolicPermissions->isChecked()) {
        settings->setUsePermissionsFormat(ContentDisplaySettings::EnumUsePermissionsFormat::SymbolicFormat);
    } else if (m_useNumericPermissions->isChecked()) {
        settings->setUsePermissionsFormat(ContentDisplaySettings::EnumUsePermissionsFormat::NumericFormat);
    } else if (m_useCombinedPermissions->isChecked()) {
        settings->setUsePermissionsFormat(ContentDisplaySettings::EnumUsePermissionsFormat::CombinedFormat);
    }
    settings->save();
}

void ContentDisplayTab::loadSettings()
{
    auto settings = ContentDisplaySettings::self();
#ifndef Q_OS_WIN
    m_numberOfItems->setChecked(settings->directorySizeMode() == ContentDisplaySettings::EnumDirectorySizeMode::ContentCount);
    m_sizeOfContents->setChecked(settings->directorySizeMode() == ContentDisplaySettings::EnumDirectorySizeMode::ContentSize);
    m_noDirectorySize->setChecked(settings->directorySizeMode() == ContentDisplaySettings::EnumDirectorySizeMode::None);
    m_recursiveDirectorySizeLimit->setValue(settings->recursiveDirectorySizeLimit());
#endif
    m_useRelatetiveDates->setChecked(settings->useShortRelativeDates());
    m_useShortDates->setChecked(!settings->useShortRelativeDates());
    m_useSymbolicPermissions->setChecked(settings->usePermissionsFormat() == ContentDisplaySettings::EnumUsePermissionsFormat::SymbolicFormat);
    m_useNumericPermissions->setChecked(settings->usePermissionsFormat() == ContentDisplaySettings::EnumUsePermissionsFormat::NumericFormat);
    m_useCombinedPermissions->setChecked(settings->usePermissionsFormat() == ContentDisplaySettings::EnumUsePermissionsFormat::CombinedFormat);
    loadSortingChoiceSettings();
}

void ContentDisplayTab::setSortingChoiceValue()
{
    auto settings = GeneralSettings::self();
    using Choice = GeneralSettings::EnumSortingChoice;
    if (m_naturalSorting->isChecked()) {
        settings->setSortingChoice(Choice::NaturalSorting);
    } else if (m_caseInsensitiveSorting->isChecked()) {
        settings->setSortingChoice(Choice::CaseInsensitiveSorting);
    } else if (m_caseSensitiveSorting->isChecked()) {
        settings->setSortingChoice(Choice::CaseSensitiveSorting);
    }
}

void ContentDisplayTab::loadSortingChoiceSettings()
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

void ContentDisplayTab::restoreDefaults()
{
    auto settings = ContentDisplaySettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

#include "moc_contentdisplaytab.cpp"
