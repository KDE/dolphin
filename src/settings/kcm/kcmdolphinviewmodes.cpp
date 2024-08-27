/*
 * SPDX-FileCopyrightText: 2008 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcmdolphinviewmodes.h"

#include "settings/viewmodes/viewsettingstab.h"

#include <KCModule>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QIcon>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

K_PLUGIN_CLASS_WITH_JSON(DolphinViewModesConfigModule, "kcmdolphinviewmodes.json")

DolphinViewModesConfigModule::DolphinViewModesConfigModule(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
    , m_tabs()
{
    setButtons(KCModule::Default | KCModule::Help | KCModule::Apply);

    QVBoxLayout *topLayout = new QVBoxLayout(widget());
    topLayout->setContentsMargins(0, 0, 0, 0);

    QTabWidget *tabWidget = new QTabWidget(widget());
    tabWidget->setDocumentMode(true);
    tabWidget->tabBar()->setExpanding(true);

    // Initialize 'Icons' tab
    ViewSettingsTab *iconsTab = new ViewSettingsTab(ViewSettingsTab::IconsMode, tabWidget);
    tabWidget->addTab(iconsTab, QIcon::fromTheme(QStringLiteral("view-list-icons")), i18nc("@title:tab", "Icons"));
    connect(iconsTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    // Initialize 'Compact' tab
    ViewSettingsTab *compactTab = new ViewSettingsTab(ViewSettingsTab::CompactMode, tabWidget);
    tabWidget->addTab(compactTab, QIcon::fromTheme(QStringLiteral("view-list-details")), i18nc("@title:tab", "Compact"));
    connect(compactTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    // Initialize 'Details' tab
    ViewSettingsTab *detailsTab = new ViewSettingsTab(ViewSettingsTab::DetailsMode, tabWidget);
    tabWidget->addTab(detailsTab, QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@title:tab", "Details"));
    connect(detailsTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    m_tabs.append(iconsTab);
    m_tabs.append(compactTab);
    m_tabs.append(detailsTab);

    topLayout->addWidget(tabWidget, 0, {});
}

DolphinViewModesConfigModule::~DolphinViewModesConfigModule()
{
}

void DolphinViewModesConfigModule::save()
{
    for (ViewSettingsTab *tab : std::as_const(m_tabs)) {
        tab->applySettings();
    }
    reparseConfiguration();
}

void DolphinViewModesConfigModule::defaults()
{
    for (ViewSettingsTab *tab : std::as_const(m_tabs)) {
        tab->restoreDefaults();
    }
    reparseConfiguration();
}

void DolphinViewModesConfigModule::reparseConfiguration()
{
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
}

void DolphinViewModesConfigModule::viewModeChanged()
{
    markAsChanged();
}

#include "kcmdolphinviewmodes.moc"

#include "moc_kcmdolphinviewmodes.cpp"
