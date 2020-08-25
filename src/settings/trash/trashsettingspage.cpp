/*
 * SPDX-FileCopyrightText: 2009 Shaun Reich <shaun.reich@kdemail.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "trashsettingspage.h"

#include <KCModuleProxy>

#include <QFormLayout>

TrashSettingsPage::TrashSettingsPage(QWidget* parent) :
        SettingsPageBase(parent)
{
    QFormLayout* topLayout = new QFormLayout(this);

    m_proxy = new KCModuleProxy(QStringLiteral("kcmtrash"));
    topLayout->addRow(m_proxy);

    loadSettings();

    connect(m_proxy, QOverload<bool>::of(&KCModuleProxy::changed), this, &TrashSettingsPage::changed);
}

TrashSettingsPage::~TrashSettingsPage()
{
}

void TrashSettingsPage::applySettings()
{
    m_proxy->save();
}

void TrashSettingsPage::restoreDefaults()
{
    m_proxy->defaults();
}

void TrashSettingsPage::loadSettings()
{
    m_proxy->load();
}

