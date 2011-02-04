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

#include "columnviewsettingspage.h"
#include "iconsviewsettingspage.h"
#include "detailsviewsettingspage.h"

#include <QVBoxLayout>

#include <KDialog>
#include <KLocale>
#include <KIconLoader>
#include <KTabWidget>

ViewSettingsPage::ViewSettingsPage(QWidget* parent) :
    SettingsPageBase(parent),
    m_pages()
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint());

    KTabWidget* tabWidget = new KTabWidget(this);

    // initialize 'Icons' tab
    IconsViewSettingsPage* iconsPage = new IconsViewSettingsPage(tabWidget);
    tabWidget->addTab(iconsPage, KIcon("view-list-icons"), i18nc("@title:tab", "Icons"));
    connect(iconsPage, SIGNAL(changed()), this, SIGNAL(changed()));

    // initialize 'Details' tab
    DetailsViewSettingsPage* detailsPage = new DetailsViewSettingsPage(tabWidget);
    tabWidget->addTab(detailsPage, KIcon("view-list-details"), i18nc("@title:tab", "Details"));
    connect(detailsPage, SIGNAL(changed()), this, SIGNAL(changed()));

    // initialize 'Column' tab
    ColumnViewSettingsPage* columnPage = new ColumnViewSettingsPage(tabWidget);
    tabWidget->addTab(columnPage, KIcon("view-file-columns"), i18nc("@title:tab", "Column"));
    connect(columnPage, SIGNAL(changed()), this, SIGNAL(changed()));

    m_pages.append(iconsPage);
    m_pages.append(detailsPage);
    m_pages.append(columnPage);

    topLayout->addWidget(tabWidget, 0, 0);
}

ViewSettingsPage::~ViewSettingsPage()
{
}

void ViewSettingsPage::applySettings()
{
    foreach (ViewSettingsPageBase* page, m_pages) {
        page->applySettings();
    }
}

void ViewSettingsPage::restoreDefaults()
{
    foreach (ViewSettingsPageBase* page, m_pages) {
        page->restoreDefaults();
    }
}

#include "viewsettingspage.moc"
