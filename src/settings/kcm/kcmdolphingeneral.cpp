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

#include "kcmdolphingeneral.h"

#include "settings/general/behaviorsettingspage.h"
#include "settings/general/previewssettingspage.h"
#include "settings/general/confirmationssettingspage.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluginLoader>
#include <kconfigwidgets_version.h>

#include <QTabWidget>
#include <QVBoxLayout>

K_PLUGIN_FACTORY(KCMDolphinGeneralConfigFactory, registerPlugin<DolphinGeneralConfigModule>(QStringLiteral("dolphingeneral"));)

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
