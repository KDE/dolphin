/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcmdolphinnavigation.h"

#include "settings/navigation/navigationsettingspage.h"
#include <kconfigwidgets_version.h>

#include <KPluginFactory>

#include <QVBoxLayout>

K_PLUGIN_CLASS_WITH_JSON(DolphinNavigationConfigModule, "kcmdolphinnavigation.json")

DolphinNavigationConfigModule::DolphinNavigationConfigModule(QObject *parent)
    : KCModule(qobject_cast<QWidget *>(parent))
    , m_navigation(nullptr)
{
    setButtons(KCModule::Default | KCModule::Help | KCModule::Apply);

    const auto parentWidget = qobject_cast<QWidget *>(parent);
    QVBoxLayout *topLayout = new QVBoxLayout(parentWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);

    m_navigation = new NavigationSettingsPage(parentWidget);
    connect(m_navigation, &NavigationSettingsPage::changed, this, &DolphinNavigationConfigModule::markAsChanged);
    topLayout->addWidget(m_navigation, 0, {});
}

DolphinNavigationConfigModule::~DolphinNavigationConfigModule()
{
}

void DolphinNavigationConfigModule::save()
{
    m_navigation->applySettings();
}

void DolphinNavigationConfigModule::defaults()
{
    m_navigation->restoreDefaults();
}

#include "kcmdolphinnavigation.moc"
