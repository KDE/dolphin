/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kcmdolphinviewmodes.h"

#include <KTabWidget>
#include <KDialog>
#include <KLocale>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KIcon>

#include <settings/viewmodes/viewsettingstab.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QPushButton>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinViewModesConfigFactory, registerPlugin<DolphinViewModesConfigModule>("dolphinviewmodes");)
K_EXPORT_PLUGIN(KCMDolphinViewModesConfigFactory("kcmdolphinviewmodes"))

DolphinViewModesConfigModule::DolphinViewModesConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(KCMDolphinViewModesConfigFactory::componentData(), parent),
    m_tabs()
{
    Q_UNUSED(args);

    KGlobal::locale()->insertCatalog("dolphin");

    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    KTabWidget* tabWidget = new KTabWidget(this);

    // Initialize 'Icons' tab
    ViewSettingsTab* iconsTab = new ViewSettingsTab(ViewSettingsTab::IconsMode, tabWidget);
    tabWidget->addTab(iconsTab, KIcon("view-list-icons"), i18nc("@title:tab", "Icons"));
    connect(iconsTab, SIGNAL(changed()), this, SLOT(viewModeChanged()));

    // Initialize 'Compact' tab
    ViewSettingsTab* compactTab = new ViewSettingsTab(ViewSettingsTab::CompactMode, tabWidget);
    tabWidget->addTab(compactTab, KIcon("view-list-details"), i18nc("@title:tab", "Compact"));
    connect(compactTab, SIGNAL(changed()), this, SLOT(viewModeChanged()));

    // Initialize 'Details' tab
    ViewSettingsTab* detailsTab = new ViewSettingsTab(ViewSettingsTab::DetailsMode, tabWidget);
    tabWidget->addTab(detailsTab, KIcon("view-list-tree"), i18nc("@title:tab", "Details"));
    connect(detailsTab, SIGNAL(changed()), this, SLOT(viewModeChanged()));

    m_tabs.append(iconsTab);
    m_tabs.append(compactTab);
    m_tabs.append(detailsTab);

    topLayout->addWidget(tabWidget, 0, 0);
}

DolphinViewModesConfigModule::~DolphinViewModesConfigModule()
{
}

void DolphinViewModesConfigModule::save()
{
    foreach (ViewSettingsTab* tab, m_tabs) {
        tab->applySettings();
    }
    reparseConfiguration();
}

void DolphinViewModesConfigModule::defaults()
{
    foreach (ViewSettingsTab* tab, m_tabs) {
        tab->restoreDefaultSettings();
    }
    reparseConfiguration();
}

void DolphinViewModesConfigModule::reparseConfiguration()
{
    QDBusMessage message = QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);
}

void DolphinViewModesConfigModule::viewModeChanged()
{
    emit changed(true);
}

#include "kcmdolphinviewmodes.moc"
