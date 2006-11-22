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
#include <kiconloader.h>
#include "generalsettingspage.h"
#include "viewsettingspage.h"
#include "bookmarkssettingspage.h"
#include "dolphin.h"
//Added by qt3to4:
#include <Q3Frame>

DolphinSettingsDialog::DolphinSettingsDialog() :
    KDialogBase(IconList, i18n("Dolphin Preferences"),
                Ok|Apply|Cancel, Ok)
{
    KIconLoader iconLoader;
    QFrame* generalSettingsFrame = addPage(i18n("General"), 0,
                                                iconLoader.loadIcon("exec",
                                                                    K3Icon::NoGroup,
                                                                    K3Icon::SizeMedium));
    m_generalSettingsPage = new GeneralSettingsPage(generalSettingsFrame);

    QFrame* viewSettingsFrame = addPage(i18n("View Modes"), 0,
                                        iconLoader.loadIcon("view_choose",
                                                            K3Icon::NoGroup,
                                                            K3Icon::SizeMedium));
    m_viewSettingsPage = new ViewSettingsPage(viewSettingsFrame);

    QFrame* bookmarksSettingsFrame = addPage(i18n("Bookmarks"), 0,
                                             iconLoader.loadIcon("bookmark",
                                                                 K3Icon::NoGroup,
                                                                 K3Icon::SizeMedium));
    m_bookmarksSettingsPage = new BookmarksSettingsPage(bookmarksSettingsFrame);
}

DolphinSettingsDialog::~DolphinSettingsDialog()
{
}

void DolphinSettingsDialog::slotOk()
{
    applySettings();
    KDialogBase::slotOk();
}

void DolphinSettingsDialog::slotApply()
{
    applySettings();
    KDialogBase::slotApply();
}

void DolphinSettingsDialog::applySettings()
{
    m_generalSettingsPage->applySettings();
    m_viewSettingsPage->applySettings();
    m_bookmarksSettingsPage->applySettings();
    Dolphin::mainWin().refreshViews();
}

#include "dolphinsettingsdialog.moc"
