/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "generalsettingspage.h"

#include "behaviorsettingspage.h"
#include "contextmenusettingspage.h"
#include "previewssettingspage.h"
#include <settings/settingspagebase.h>
#include "statusbarsettingspage.h"

#include <KDialog>
#include <KLocale>
#include <KIconLoader>
#include <KTabWidget>

#include <QVBoxLayout>

GeneralSettingsPage::GeneralSettingsPage(const KUrl& url, QWidget* parent) :
    SettingsPageBase(parent),
    m_pages()
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    KTabWidget* tabWidget = new KTabWidget(this);

    // initialize 'Behavior' tab
    BehaviorSettingsPage* behaviorPage = new BehaviorSettingsPage(url, tabWidget);
    tabWidget->addTab(behaviorPage, i18nc("@title:tab Behavior settings", "Behavior"));
    connect(behaviorPage, SIGNAL(changed()), this, SIGNAL(changed()));

    // initialize 'Previews' tab
    PreviewsSettingsPage* previewsPage = new PreviewsSettingsPage(tabWidget);
    tabWidget->addTab(previewsPage, i18nc("@title:tab Previews settings", "Previews"));
    connect(previewsPage, SIGNAL(changed()), this, SIGNAL(changed()));

    // initialize 'Context Menu' tab
    ContextMenuSettingsPage* contextMenuPage = new ContextMenuSettingsPage(tabWidget);
    tabWidget->addTab(contextMenuPage, i18nc("@title:tab Context Menu settings", "Context Menu"));
    connect(contextMenuPage, SIGNAL(changed()), this, SIGNAL(changed()));

    // initialize 'Status Bar' tab
    StatusBarSettingsPage* statusBarPage = new StatusBarSettingsPage(tabWidget);
    tabWidget->addTab(statusBarPage, i18nc("@title:tab Status Bar settings", "Status Bar"));
    connect(statusBarPage, SIGNAL(changed()), this, SIGNAL(changed()));

    m_pages.append(behaviorPage);
    m_pages.append(previewsPage);
    m_pages.append(contextMenuPage);
    m_pages.append(statusBarPage);

    topLayout->addWidget(tabWidget, 0, 0);
}

GeneralSettingsPage::~GeneralSettingsPage()
{
}

void GeneralSettingsPage::applySettings()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->applySettings();
    }
}

void GeneralSettingsPage::restoreDefaults()
{
    foreach (SettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }
}

#include "generalsettingspage.moc"
