/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcmdolphingeneral.h"

#include "settings/interface/confirmationssettingspage.h"
#include "settings/interface/folderstabssettingspage.h"
#include "settings/interface/interfacesettingspage.h"
#include "settings/interface/previewssettingspage.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <kconfigwidgets_version.h>

#include <QTabWidget>
#include <QVBoxLayout>

K_PLUGIN_CLASS_WITH_JSON(DolphinGeneralConfigModule, "kcmdolphingeneral.json")

DolphinGeneralConfigModule::DolphinGeneralConfigModule(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
    , m_pages()
{
    setButtons(KCModule::Default | KCModule::Help | KCModule::Apply);

    QVBoxLayout *topLayout = new QVBoxLayout(widget());
    topLayout->setContentsMargins(0, 0, 0, 0);

    QTabWidget *tabWidget = new QTabWidget(widget());

    // initialize 'Folders & Tabs' tab
    FoldersTabsSettingsPage *foldersTabsPage = new FoldersTabsSettingsPage(tabWidget);
    tabWidget->addTab(foldersTabsPage, i18nc("@title:tab Behavior settings", "Behavior"));
    connect(foldersTabsPage, &FoldersTabsSettingsPage::changed, this, &DolphinGeneralConfigModule::markAsChanged);

    // initialize 'Previews' tab
    PreviewsSettingsPage *previewsPage = new PreviewsSettingsPage(tabWidget);
    tabWidget->addTab(previewsPage, i18nc("@title:tab Previews settings", "Previews"));
    connect(previewsPage, &PreviewsSettingsPage::changed, this, &DolphinGeneralConfigModule::markAsChanged);

    // initialize 'Confirmations' tab
    ConfirmationsSettingsPage *confirmationsPage = new ConfirmationsSettingsPage(tabWidget);
    tabWidget->addTab(confirmationsPage, i18nc("@title:tab Confirmations settings", "Confirmations"));
    connect(confirmationsPage, &ConfirmationsSettingsPage::changed, this, &DolphinGeneralConfigModule::markAsChanged);
    m_pages.append(foldersTabsPage);
    m_pages.append(previewsPage);
    m_pages.append(confirmationsPage);

    topLayout->addWidget(tabWidget, 0, {});
}

DolphinGeneralConfigModule::~DolphinGeneralConfigModule()
{
}

void DolphinGeneralConfigModule::save()
{
    for (SettingsPageBase *page : std::as_const(m_pages)) {
        page->applySettings();
    }
}

void DolphinGeneralConfigModule::defaults()
{
    for (SettingsPageBase *page : std::as_const(m_pages)) {
        page->applySettings();
    }
}

#include "kcmdolphingeneral.moc"

#include "moc_kcmdolphingeneral.cpp"
