/*
 * SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "searchsettingspage.h"

#include "dolphin_generalsettings.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QVBoxLayout>

SearchSettingsPage::SearchSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
    , m_useIndexing(nullptr)
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setAlignment(Qt::AlignTop);

    m_useIndexing = new QCheckBox(i18nc("@option:check Use the indexing service when searching", "Use indexing service"), this);

    topLayout->addWidget(m_useIndexing);

    loadSettings();

    connect(m_useIndexing, &QCheckBox::toggled, this, &SearchSettingsPage::changed);
}

SearchSettingsPage::~SearchSettingsPage()
{
}

void SearchSettingsPage::applySettings()
{
    GeneralSettings *settings = GeneralSettings::self();
    settings->setUseIndexing(m_useIndexing->isChecked());
    settings->save();
}

void SearchSettingsPage::restoreDefaults()
{
    GeneralSettings *settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void SearchSettingsPage::loadSettings()
{
    m_useIndexing->setChecked(GeneralSettings::useIndexing());
}

#include "searchsettingspage.moc"
