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

#include "dolphinsettingsdialog.h"
#include <klocale.h>
#include <kicon.h>
#include "generalsettingspage.h"
#include "viewsettingspage.h"
#include "bookmarkssettingspage.h"
#include "dolphin.h"
//Added by qt3to4:
#include <QFrame>

DolphinSettingsDialog::DolphinSettingsDialog() :
    KPageDialog()
{
    setFaceType( List);
    setCaption(i18n("Dolphin Preferences"));
    setButtons(Ok|Apply|Cancel);
    setDefaultButton(Ok);

    m_generalSettingsPage = new GeneralSettingsPage(this);
    KPageWidgetItem* generalSettingsFrame = addPage(m_generalSettingsPage, i18n("General"));
    generalSettingsFrame->setIcon(KIcon("exec"));

    m_viewSettingsPage = new ViewSettingsPage(this);
    KPageWidgetItem* viewSettingsFrame = addPage(m_viewSettingsPage, i18n("View Modes"));
    viewSettingsFrame->setIcon(KIcon("view_choose"));

    m_bookmarksSettingsPage = new BookmarksSettingsPage(this);
    KPageWidgetItem* bookmarksSettingsFrame = addPage(m_bookmarksSettingsPage, i18n("Bookmarks"));
    bookmarksSettingsFrame->setIcon(KIcon("bookmark"));
}

DolphinSettingsDialog::~DolphinSettingsDialog()
{
}

void DolphinSettingsDialog::slotButtonClicked(int button)
{
    if (button==Ok || button==Apply) {
        applySettings();
    }
    KPageDialog::slotButtonClicked(button);
}

void DolphinSettingsDialog::applySettings()
{
    m_generalSettingsPage->applySettings();
    m_viewSettingsPage->applySettings();
    m_bookmarksSettingsPage->applySettings();
    Dolphin::mainWin().refreshViews();
}

#include "dolphinsettingsdialog.moc"
