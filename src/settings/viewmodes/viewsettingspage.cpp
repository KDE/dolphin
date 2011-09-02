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

#include "viewsettingspage.h"

#include <views/dolphinview.h>
#include "viewsettingstab.h"

#include <QVBoxLayout>

#include <KDialog>
#include <KLocale>
#include <KIconLoader>
#include <KTabWidget>

ViewSettingsPage::ViewSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_tabs()
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    KTabWidget* tabWidget = new KTabWidget(this);

    // Initialize 'Icons' tab
    ViewSettingsTab* iconsTab = new ViewSettingsTab(ViewSettingsTab::IconsMode, tabWidget);
    tabWidget->addTab(iconsTab, KIcon("view-list-icons"), i18nc("@title:tab", "Icons"));
    connect(iconsTab, SIGNAL(changed()), this, SIGNAL(changed()));

    // Initialize 'Compact' tab
    ViewSettingsTab* compactTab = new ViewSettingsTab(ViewSettingsTab::CompactMode, tabWidget);
    tabWidget->addTab(compactTab, KIcon("view-list-details"), i18nc("@title:tab", "Compact"));
    connect(compactTab, SIGNAL(changed()), this, SIGNAL(changed()));

    // Initialize 'Details' tab
    ViewSettingsTab* detailsTab = new ViewSettingsTab(ViewSettingsTab::DetailsMode, tabWidget);
    tabWidget->addTab(detailsTab, KIcon("view-list-tree"), i18nc("@title:tab", "Details"));
    connect(detailsTab, SIGNAL(changed()), this, SIGNAL(changed()));

    m_tabs.append(iconsTab);
    m_tabs.append(compactTab);
    m_tabs.append(detailsTab);

    topLayout->addWidget(tabWidget, 0, 0);
}

ViewSettingsPage::~ViewSettingsPage()
{
}

void ViewSettingsPage::applySettings()
{
    foreach (ViewSettingsTab* tab, m_tabs) {
        tab->applySettings();
    }
}

void ViewSettingsPage::restoreDefaults()
{
    foreach (ViewSettingsTab* tab, m_tabs) {
        tab->restoreDefaultSettings();
    }
}

#include "viewsettingspage.moc"
