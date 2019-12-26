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

#include "kcmdolphinnavigation.h"

#include "settings/navigation/navigationsettingspage.h"
#include <kconfigwidgets_version.h>

#include <KPluginFactory>
#include <KPluginLoader>

#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinNavigationConfigFactory, registerPlugin<DolphinNavigationConfigModule>(QStringLiteral("dolphinnavigation"));)

DolphinNavigationConfigModule::DolphinNavigationConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(parent),
    m_navigation(nullptr)
{
    Q_UNUSED(args)

    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    m_navigation = new NavigationSettingsPage(this);
#if KCONFIGWIDGETS_VERSION < QT_VERSION_CHECK(5, 64, 0)
    connect(m_navigation, &NavigationSettingsPage::changed, this, QOverload<>::of(&DolphinNavigationConfigModule::changed));
#else
    connect(m_navigation, &NavigationSettingsPage::changed, this, &DolphinNavigationConfigModule::markAsChanged);
#endif
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
