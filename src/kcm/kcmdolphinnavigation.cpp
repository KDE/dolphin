/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "kcmdolphinnavigation.h"

#include "settings/navigationsettingspage.h"

#include <ktabwidget.h>
#include <kdialog.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinNavigationConfigFactory, registerPlugin<DolphinNavigationConfigModule>("dolphinnavigation");)
K_EXPORT_PLUGIN(KCMDolphinNavigationConfigFactory("kcmdolphinnavigation"))

DolphinNavigationConfigModule::DolphinNavigationConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(KCMDolphinNavigationConfigFactory::componentData(), parent),
    m_navigation(0)
{
    Q_UNUSED(args);

    KGlobal::locale()->insertCatalog("dolphinnavigation");

    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    m_navigation = new NavigationSettingsPage(this);
    topLayout->addWidget(m_navigation, 0, 0);
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
    m_navigation->applySettings();
}

#include "kcmdolphinnavigation.moc"
