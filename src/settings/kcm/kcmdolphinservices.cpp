/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kcmdolphinservices.h"

#include "settings/services/servicessettingspage.h"

#include <KPluginFactory>
#include <KPluginLoader>

#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinServicesConfigFactory, registerPlugin<DolphinServicesConfigModule>(QStringLiteral("dolphinservices"));)

DolphinServicesConfigModule::DolphinServicesConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(parent),
    m_services(nullptr)
{
    Q_UNUSED(args);

    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);

    m_services = new ServicesSettingsPage(this);
    connect(m_services, &ServicesSettingsPage::changed, this, static_cast<void(DolphinServicesConfigModule::*)()>(&DolphinServicesConfigModule::changed));
    topLayout->addWidget(m_services, 0, nullptr);
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
