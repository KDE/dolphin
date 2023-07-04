/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "generalsettingspage.h"

#include "behaviorsettingspage.h"
#include "confirmationssettingspage.h"
#include "previewssettingspage.h"
#include "statusbarsettingspage.h"

#include <KLocalizedString>

#include <QTabWidget>
#include <QVBoxLayout>

GeneralSettingsPage::GeneralSettingsPage(const QUrl &url, QWidget *parent)
    : SettingsPageBase(parent)
    , m_pages()
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    QTabWidget *tabWidget = new QTabWidget(this);

    // initialize 'Behavior' tab
    BehaviorSettingsPage *behaviorPage = new BehaviorSettingsPage(url, tabWidget);
    tabWidget->addTab(behaviorPage, i18nc("@title:tab Behavior settings", "Behavior"));
    connect(behaviorPage, &BehaviorSettingsPage::changed, this, &GeneralSettingsPage::changed);

    // initialize 'Previews' tab
    PreviewsSettingsPage *previewsPage = new PreviewsSettingsPage(tabWidget);
    tabWidget->addTab(previewsPage, i18nc("@title:tab Previews settings", "Previews"));
    connect(previewsPage, &PreviewsSettingsPage::changed, this, &GeneralSettingsPage::changed);

    // initialize 'Context Menu' tab
    ConfirmationsSettingsPage *confirmationsPage = new ConfirmationsSettingsPage(tabWidget);
    tabWidget->addTab(confirmationsPage, i18nc("@title:tab Confirmations settings", "Confirmations"));
    connect(confirmationsPage, &ConfirmationsSettingsPage::changed, this, &GeneralSettingsPage::changed);

    // initialize 'Status Bar' tab
    StatusBarSettingsPage *statusBarPage = new StatusBarSettingsPage(tabWidget);
    tabWidget->addTab(statusBarPage, i18nc("@title:tab Status Bar settings", "Status Bar"));
    connect(statusBarPage, &StatusBarSettingsPage::changed, this, &GeneralSettingsPage::changed);

    m_pages.append(behaviorPage);
    m_pages.append(previewsPage);
    m_pages.append(confirmationsPage);
    m_pages.append(statusBarPage);

    topLayout->addWidget(tabWidget, 0, {});
}

GeneralSettingsPage::~GeneralSettingsPage()
{
}

void GeneralSettingsPage::applySettings()
{
    for (SettingsPageBase *page : qAsConst(m_pages)) {
        page->applySettings();
    }
}

void GeneralSettingsPage::restoreDefaults()
{
    for (SettingsPageBase *page : qAsConst(m_pages)) {
        page->restoreDefaults();
    }
}

#include "moc_generalsettingspage.cpp"
