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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "viewsettingspage.h"
#include "generalviewsettingspage.h"
#include "iconsviewsettingspage.h"
#include "detailsviewsettingspage.h"

#include <Q3VBoxLayout>
#include <QTabWidget>
#include <QLayout>
#include <QLabel>

#include <kdialog.h>
#include <klocale.h>
#include <kiconloader.h>

ViewSettingsPage::ViewSettingsPage(DolphinMainWindow* mainWindow,
                                   QWidget* parent) :
    SettingsPageBase(parent),
    m_generalPage(0),
    m_iconsPage(0),
    m_detailsPage(0)
{
    Q3VBoxLayout* topLayout = new Q3VBoxLayout(this, 0, KDialog::spacingHint());

    QTabWidget* tabWidget = new QTabWidget(this);

    // initialize 'General' tab
    m_generalPage = new GeneralViewSettingsPage(mainWindow, tabWidget);
    tabWidget->addTab(m_generalPage, SmallIcon("view_choose"), i18n("General"));

    // initialize 'Icons' tab
    m_iconsPage = new IconsViewSettingsPage(mainWindow, tabWidget);
    tabWidget->addTab(m_iconsPage, SmallIcon("view_icon"), i18n("Icons"));

    // initialize 'Details' tab
    m_detailsPage = new DetailsViewSettingsPage(mainWindow, tabWidget);
    tabWidget->addTab(m_detailsPage, SmallIcon("view_text"), i18n("Details"));

    topLayout->addWidget(tabWidget, 0, 0 );
}

ViewSettingsPage::~ViewSettingsPage()
{
}

void ViewSettingsPage::applySettings()
{
    m_generalPage->applySettings();
    m_iconsPage->applySettings();
    m_detailsPage->applySettings();
}

#include "viewsettingspage.moc"
