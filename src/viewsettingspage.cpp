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
#include "generalviewsettingspage.h"
#include "iconsviewsettingspage.h"
#include "detailsviewsettingspage.h"

#include <QVBoxLayout>
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
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(KDialog::spacingHint()));

    QTabWidget* tabWidget = new QTabWidget(this);

    // initialize 'General' tab
    m_generalPage = new GeneralViewSettingsPage(mainWindow, tabWidget);
    tabWidget->addTab(m_generalPage, SmallIcon("view-choose"), i18n("General"));

    // initialize 'Icons' tab
    m_iconsPage = new IconsViewSettingsPage(mainWindow, tabWidget);
    tabWidget->addTab(m_iconsPage, SmallIcon("view-icon"), i18n("Icons"));

    // initialize 'Details' tab
    m_detailsPage = new DetailsViewSettingsPage(mainWindow, tabWidget);
    tabWidget->addTab(m_detailsPage, SmallIcon("fileview-text"), i18n("Details"));

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
