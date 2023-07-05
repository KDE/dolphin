/*
 * SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "contentdisplaytab.h"
#include "dolphin_contentdisplaysettings.h"

#include <KFormat>
#include <KLocalizedString>

#include <QButtonGroup>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QSpinBox>

ContentDisplayTab::ContentDisplayTab(QWidget *parent)
    : SettingsPageBase(parent)
    , m_numberOfItems(nullptr)
    , m_sizeOfContents(nullptr)
    , m_recursiveDirectorySizeLimit(nullptr)
    , m_useRelatetiveDates(nullptr)
    , m_useShortDates(nullptr)
    , m_useSymbolicPermissions(nullptr)
    , m_useNumericPermissions(nullptr)
    , m_useCombinedPermissions(nullptr)
{
    QFormLayout *topLayout = new QFormLayout(this);

#ifndef Q_OS_WIN
    // Sorting properties
    m_numberOfItems = new QRadioButton(i18nc("option:radio", "Number of items"));
    m_sizeOfContents = new QRadioButton(i18nc("option:radio", "Size of contents, up to "));

    QButtonGroup *sortingModeGroup = new QButtonGroup(this);
    sortingModeGroup->addButton(m_numberOfItems);
    sortingModeGroup->addButton(m_sizeOfContents);

    m_recursiveDirectorySizeLimit = new QSpinBox();
    connect(m_recursiveDirectorySizeLimit, &QSpinBox::valueChanged, this, [this](int value) {
        m_recursiveDirectorySizeLimit->setSuffix(i18np(" level deep", " levels deep", value));
    });
    m_recursiveDirectorySizeLimit->setRange(1, 20);
    m_recursiveDirectorySizeLimit->setSingleStep(1);

    QHBoxLayout *contentsSizeLayout = new QHBoxLayout();
    contentsSizeLayout->addWidget(m_sizeOfContents);
    contentsSizeLayout->addWidget(m_recursiveDirectorySizeLimit);

    topLayout->addRow(i18nc("@title:group", "Folder size displays:"), m_numberOfItems);
    topLayout->addRow(QString(), contentsSizeLayout);
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
#endif

    connect(m_useRelatetiveDates, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_useShortDates, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_useSymbolicPermissions, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_useNumericPermissions, &QRadioButton::toggled, this, &SettingsPageBase::changed);
    connect(m_useCombinedPermissions, &QRadioButton::toggled, this, &SettingsPageBase::changed);

    loadSettings();
}

void ContentDisplayTab::applySettings()
{
    auto settings = ContentDisplaySettings::self();
#ifndef Q_OS_WIN
    settings->setDirectorySizeCount(m_numberOfItems->isChecked());
    settings->setRecursiveDirectorySizeLimit(m_recursiveDirectorySizeLimit->value());
#endif

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
    if (settings->directorySizeCount()) {
        m_numberOfItems->setChecked(true);
        m_recursiveDirectorySizeLimit->setEnabled(false);
    } else {
        m_sizeOfContents->setChecked(true);
        m_recursiveDirectorySizeLimit->setEnabled(true);
    }
    m_recursiveDirectorySizeLimit->setValue(settings->recursiveDirectorySizeLimit());
#endif
    m_useRelatetiveDates->setChecked(settings->useShortRelativeDates());
    m_useShortDates->setChecked(!settings->useShortRelativeDates());
    m_useSymbolicPermissions->setChecked(settings->usePermissionsFormat() == ContentDisplaySettings::EnumUsePermissionsFormat::SymbolicFormat);
    m_useNumericPermissions->setChecked(settings->usePermissionsFormat() == ContentDisplaySettings::EnumUsePermissionsFormat::NumericFormat);
    m_useCombinedPermissions->setChecked(settings->usePermissionsFormat() == ContentDisplaySettings::EnumUsePermissionsFormat::CombinedFormat);
}

void ContentDisplayTab::restoreDefaults()
{
    auto settings = ContentDisplaySettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

#include "moc_contentdisplaytab.cpp"
