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

#include "settings/viewmodes/viewsettingstab.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluginLoader>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinViewModesConfigFactory, registerPlugin<DolphinViewModesConfigModule>(QStringLiteral("dolphinviewmodes"));)

DolphinViewModesConfigModule::DolphinViewModesConfigModule(QWidget* parent, const QVariantList& args) :
    KCModule(parent),
    m_tabs()
{
    Q_UNUSED(args);

    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);

    QTabWidget* tabWidget = new QTabWidget(this);

    // Initialize 'Icons' tab
    ViewSettingsTab* iconsTab = new ViewSettingsTab(ViewSettingsTab::IconsMode, tabWidget);
    tabWidget->addTab(iconsTab, QIcon::fromTheme(QStringLiteral("view-list-icons")), i18nc("@title:tab", "Icons"));
    connect(iconsTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    // Initialize 'Compact' tab
    ViewSettingsTab* compactTab = new ViewSettingsTab(ViewSettingsTab::CompactMode, tabWidget);
    tabWidget->addTab(compactTab, QIcon::fromTheme(QStringLiteral("view-list-details")), i18nc("@title:tab", "Compact"));
    connect(compactTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    // Initialize 'Details' tab
    ViewSettingsTab* detailsTab = new ViewSettingsTab(ViewSettingsTab::DetailsMode, tabWidget);
    tabWidget->addTab(detailsTab, QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@title:tab", "Details"));
    connect(detailsTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    m_tabs.append(iconsTab);
    m_tabs.append(compactTab);
    m_tabs.append(detailsTab);

    topLayout->addWidget(tabWidget, 0, nullptr);
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
    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
}

void DolphinViewModesConfigModule::viewModeChanged()
{
    emit changed(true);
}

#include "kcmdolphinviewmodes.moc"
