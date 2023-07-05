/*
 * SPDX-FileCopyrightText: 2009 Shaun Reich <shaun.reich@kdemail.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "trashsettingspage.h"

#include <KCModuleLoader>
#include <KCModule>
#include <KPluginMetaData>

#include <QFormLayout>

TrashSettingsPage::TrashSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
{
    QFormLayout *topLayout = new QFormLayout(this);

    m_kcm = KCModuleLoader::loadModule(KPluginMetaData(QStringLiteral("kcm_trash")));

    topLayout->addRow(m_kcm->widget());

    loadSettings();

    connect(m_kcm, &KCModule::needsSaveChanged, this, &TrashSettingsPage::changed);
}

TrashSettingsPage::~TrashSettingsPage()
{
}

void TrashSettingsPage::applySettings()
{
    m_kcm->save();
}

void TrashSettingsPage::restoreDefaults()
{
    m_kcm->defaults();
}

void TrashSettingsPage::loadSettings()
{
    m_kcm->load();
}

#include "moc_trashsettingspage.cpp"
