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

#include <KTabWidget>
#include <KDialog>
#include <KLocale>
#include <KPluginFactory>
#include <KPluginLoader>

#include <settings/services/servicessettingspage.h>

#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinServicesConfigFactory, registerPlugin<DolphinServicesConfigModule>("dolphinservices");)
K_EXPORT_PLUGIN(KCMDolphinServicesConfigFactory("kcmdolphinservices"))

DolphinServicesConfigModule::DolphinServicesConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(KCMDolphinServicesConfigFactory::componentData(), parent),
    m_services(0)
{
    Q_UNUSED(args);

    KGlobal::locale()->insertCatalog("dolphin");

    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    m_services = new ServicesSettingsPage(this);
    connect(m_services, SIGNAL(changed()), this, SLOT(changed()));
    topLayout->addWidget(m_services, 0, 0);
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
