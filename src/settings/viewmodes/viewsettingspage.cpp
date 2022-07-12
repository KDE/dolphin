/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "viewsettingspage.h"

#include "views/dolphinview.h"

#include "vcssettingstab.h"
#include "viewsettingstab.h"

#include <KLocalizedString>
#include <KPluginMetaData>

#include <QShowEvent>
#include <QTabWidget>
#include <QVBoxLayout>

ViewSettingsPage::ViewSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_tabs()
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);

    // Initialize 'Icons' tab
    ViewSettingsTab* iconsTab = new ViewSettingsTab(ViewSettingsTab::IconsMode, m_tabWidget);
    m_tabWidget->addTab(iconsTab, QIcon::fromTheme(QStringLiteral("view-list-icons")), i18nc("@title:tab", "Icons"));
    connect(iconsTab, &ViewSettingsTab::changed, this, &ViewSettingsPage::changed);

    // Initialize 'Compact' tab
    ViewSettingsTab* compactTab = new ViewSettingsTab(ViewSettingsTab::CompactMode, m_tabWidget);
    m_tabWidget->addTab(compactTab, QIcon::fromTheme(QStringLiteral("view-list-details")), i18nc("@title:tab", "Compact"));
    connect(compactTab, &ViewSettingsTab::changed, this, &ViewSettingsPage::changed);

    // Initialize 'Details' tab
    ViewSettingsTab* detailsTab = new ViewSettingsTab(ViewSettingsTab::DetailsMode, m_tabWidget);
    m_tabWidget->addTab(detailsTab, QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@title:tab", "Details"));
    connect(detailsTab, &ViewSettingsTab::changed, this, &ViewSettingsPage::changed);

    // 'Version Control' tab is initialized on-demand and only shown if there are plug-ins

    m_tabs.append(iconsTab);
    m_tabs.append(compactTab);
    m_tabs.append(detailsTab);

    topLayout->addWidget(m_tabWidget, 0);
}

ViewSettingsPage::~ViewSettingsPage()
{
}

void ViewSettingsPage::applySettings()
{
    for (ViewSettingsTab* tab : qAsConst(m_tabs)) {
        tab->applySettings();
    }
    if (m_vcsTab) {
        m_vcsTab->applySettings();
    }
}

void ViewSettingsPage::restoreDefaults()
{
    for (ViewSettingsTab* tab : qAsConst(m_tabs)) {
        tab->restoreDefaultSettings();
    }
    if (m_vcsTab) {
        m_vcsTab->restoreDefaultSettings();
    }
}

void ViewSettingsPage::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !m_vcsLoaded) {
        auto plugins = KPluginMetaData::findPlugins(QStringLiteral("dolphin/vcs"));
        std::sort(plugins.begin(), plugins.end(), [](const KPluginMetaData &a, const KPluginMetaData &b) {
            return a.name() < b.name();
        });

        if (!plugins.isEmpty()) {
            m_vcsTab = new VcsSettingsTab(plugins, m_tabWidget);
            m_tabWidget->addTab(m_vcsTab, QIcon::fromTheme(QStringLiteral("code-class")), i18nc("@title:tab", "Version Control"));
            connect(m_vcsTab, &VcsSettingsTab::changed, this, &ViewSettingsPage::changed);
        }

        m_vcsLoaded = true;
    }
}
