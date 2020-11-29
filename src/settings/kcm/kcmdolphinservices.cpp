/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcmdolphinservices.h"

#include "settings/services/servicessettingspage.h"

#include <kconfigwidgets_version.h>
#include <KPluginFactory>

#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinServicesConfigFactory, registerPlugin<DolphinServicesConfigModule>(QStringLiteral("dolphinservices"));)

DolphinServicesConfigModule::DolphinServicesConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(parent, args),
    m_services(nullptr)
{
    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    m_services = new ServicesSettingsPage(this);
    connect(m_services, &ServicesSettingsPage::changed, this, &DolphinServicesConfigModule::markAsChanged);
    topLayout->addWidget(m_services, 0, {});
}

DolphinServicesConfigModule::~DolphinServicesConfigModule()
{
}

void DolphinServicesConfigModule::save()
{
    m_services->applySettings();
}

void DolphinServicesConfigModule::defaults()
{
    m_services->restoreDefaults();
}

#include "kcmdolphinservices.moc"
