/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "interfacesettingspage.h"

#include "confirmationssettingspage.h"
#include "folderstabssettingspage.h"
#include "previewssettingspage.h"
#include "statusandlocationbarssettingspage.h"

#include <KLocalizedString>

#include <QTabWidget>
#include <QVBoxLayout>

InterfaceSettingsPage::InterfaceSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
    , m_pages()
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    QTabWidget *tabWidget = new QTabWidget(this);

    // initialize 'Folders & Tabs' tab
    FoldersTabsSettingsPage *foldersTabsPage = new FoldersTabsSettingsPage(tabWidget);
    tabWidget->addTab(foldersTabsPage, i18nc("@title:tab Folders & Tabs settings", "Folders && Tabs"));
    connect(foldersTabsPage, &FoldersTabsSettingsPage::changed, this, &InterfaceSettingsPage::changed);

    // initialize 'Previews' tab
    PreviewsSettingsPage *previewsPage = new PreviewsSettingsPage(tabWidget);
    tabWidget->addTab(previewsPage, i18nc("@title:tab Previews settings", "Previews"));
    connect(previewsPage, &PreviewsSettingsPage::changed, this, &InterfaceSettingsPage::changed);

    // initialize 'Context Menu' tab
    ConfirmationsSettingsPage *confirmationsPage = new ConfirmationsSettingsPage(tabWidget);
    tabWidget->addTab(confirmationsPage, i18nc("@title:tab Confirmations settings", "Confirmations"));
    connect(confirmationsPage, &ConfirmationsSettingsPage::changed, this, &InterfaceSettingsPage::changed);

    // initialize 'Status & location bars' tab
    StatusAndLocationBarsSettingsPage *statusAndLocationBarsPage = new StatusAndLocationBarsSettingsPage(tabWidget, foldersTabsPage);
    tabWidget->addTab(statusAndLocationBarsPage, i18nc("@title:tab Status & Location bars settings", "Status && Location bars"));
    connect(statusAndLocationBarsPage, &StatusAndLocationBarsSettingsPage::changed, this, &InterfaceSettingsPage::changed);

    m_pages.append(foldersTabsPage);
    m_pages.append(previewsPage);
    m_pages.append(confirmationsPage);
    m_pages.append(statusAndLocationBarsPage);

    topLayout->addWidget(tabWidget, 0, {});
}

InterfaceSettingsPage::~InterfaceSettingsPage()
{
}

void InterfaceSettingsPage::applySettings()
{
    for (SettingsPageBase *page : qAsConst(m_pages)) {
        page->applySettings();
    }
}

void InterfaceSettingsPage::restoreDefaults()
{
    for (SettingsPageBase *page : qAsConst(m_pages)) {
        page->restoreDefaults();
    }
}

#include "moc_interfacesettingspage.cpp"
