/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "viewsettingspage.h"

#include "contentdisplaytab.h"
#include "viewsettingstab.h"

#include <KLocalizedString>

#include <QTabWidget>
#include <QVBoxLayout>

ViewSettingsPage::ViewSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
    , m_tabs()
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    tabWidget = new QTabWidget(this);

    // Content Display Tab
    contentDisplayTab = new ContentDisplayTab(tabWidget);
    tabWidget->addTab(contentDisplayTab,
                      QIcon::fromTheme(QStringLiteral("view-choose")),
                      i18nc("@title:tab how file items columns are displayed", "Content Display"));
    connect(contentDisplayTab, &SettingsPageBase::changed, this, &ViewSettingsPage::changed);

    // Initialize 'Icons' tab
    ViewSettingsTab *iconsTab = new ViewSettingsTab(ViewSettingsTab::IconsMode, tabWidget);
    tabWidget->addTab(iconsTab, QIcon::fromTheme(QStringLiteral("view-list-icons")), i18nc("@title:tab", "Icons"));
    connect(iconsTab, &ViewSettingsTab::changed, this, &ViewSettingsPage::changed);

    // Initialize 'Compact' tab
    ViewSettingsTab *compactTab = new ViewSettingsTab(ViewSettingsTab::CompactMode, tabWidget);
    tabWidget->addTab(compactTab, QIcon::fromTheme(QStringLiteral("view-list-details")), i18nc("@title:tab", "Compact"));
    connect(compactTab, &ViewSettingsTab::changed, this, &ViewSettingsPage::changed);

    // Initialize 'Details' tab
    ViewSettingsTab *detailsTab = new ViewSettingsTab(ViewSettingsTab::DetailsMode, tabWidget);
    tabWidget->addTab(detailsTab, QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@title:tab", "Details"));
    connect(detailsTab, &ViewSettingsTab::changed, this, &ViewSettingsPage::changed);

    m_tabs.append(iconsTab);
    m_tabs.append(compactTab);
    m_tabs.append(detailsTab);

    topLayout->addWidget(tabWidget, 0);
}

ViewSettingsPage::~ViewSettingsPage()
{
}

void ViewSettingsPage::applySettings()
{
    contentDisplayTab->applySettings();

    for (ViewSettingsTab *tab : qAsConst(m_tabs)) {
        tab->applySettings();
    }
}

void ViewSettingsPage::restoreDefaults()
{
    if (tabWidget->currentWidget() == contentDisplayTab) {
        contentDisplayTab->restoreDefaults();
        return;
    }

    for (ViewSettingsTab *tab : qAsConst(m_tabs)) {
        if (tabWidget->currentWidget() == tab) {
            tab->restoreDefaultSettings();
            return;
        }
    }
}

#include "moc_viewsettingspage.cpp"
