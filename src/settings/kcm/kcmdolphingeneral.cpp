/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcmdolphingeneral.h"

#include "settings/general/behaviorsettingspage.h"
#include "settings/general/previewssettingspage.h"
#include "settings/general/confirmationssettingspage.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <kconfigwidgets_version.h>

#include <QTabWidget>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinGeneralConfigFactory, registerPlugin<DolphinGeneralConfigModule>();)

DolphinGeneralConfigModule::DolphinGeneralConfigModule(QWidget *parent, const QVariantList &args) :
    KCModule(parent, args),
    m_pages()
{
    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    QTabWidget* tabWidget = new QTabWidget(this);

    // initialize 'Behavior' tab
    BehaviorSettingsPage* behaviorPage = new BehaviorSettingsPage(QUrl::fromLocalFile(QDir::homePath()), tabWidget);
    tabWidget->addTab(behaviorPage, i18nc("@title:tab Behavior settings", "Behavior"));
    connect(behaviorPage, &BehaviorSettingsPage::changed, this, &DolphinGeneralConfigModule::markAsChanged);

    // initialize 'Previews' tab
    PreviewsSettingsPage* previewsPage = new PreviewsSettingsPage(tabWidget);
    tabWidget->addTab(previewsPage, i18nc("@title:tab Previews settings", "Previews"));
    connect(previewsPage, &PreviewsSettingsPage::changed, this, &DolphinGeneralConfigModule::markAsChanged);

    // initialize 'Confirmations' tab
    ConfirmationsSettingsPage* confirmationsPage = new ConfirmationsSettingsPage(tabWidget);
    tabWidget->addTab(confirmationsPage,  i18nc("@title:tab Confirmations settings", "Confirmations"));
    connect(confirmationsPage, &ConfirmationsSettingsPage::changed, this, &DolphinGeneralConfigModule::markAsChanged);
    m_pages.append(behaviorPage);
    m_pages.append(previewsPage);
    m_pages.append(confirmationsPage);

    topLayout->addWidget(tabWidget, 0, {});
}

DolphinGeneralConfigModule::~DolphinGeneralConfigModule()
{
}

void DolphinGeneralConfigModule::save()
{
    for (SettingsPageBase* page : qAsConst(m_pages)) {
        page->applySettings();
    }
}

void DolphinGeneralConfigModule::defaults()
{
    for (SettingsPageBase* page : qAsConst(m_pages)) {
        page->applySettings();
    }
}

#include "kcmdolphingeneral.moc"
