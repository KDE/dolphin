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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "dolphinnewmenu.h"
#include "dolphinmainwindow.h"
#include "dolphinstatusbar.h"
#include "dolphinview.h"

#include <kactioncollection.h>
#include <kio/job.h>

DolphinNewMenu::DolphinNewMenu(DolphinMainWindow* mainWin) :
    KNewMenu(mainWin->actionCollection(), mainWin, "create_new"),
    m_mainWin(mainWin)
{
}

DolphinNewMenu::~DolphinNewMenu()
{
}

void DolphinNewMenu::slotResult(KJob* job)
{
    if (job->error()) {
        DolphinStatusBar* statusBar = m_mainWin->activeView()->statusBar();
        statusBar->setMessage(job->errorString(), DolphinStatusBar::Error);
    }
    else {
        KNewMenu::slotResult(job);
    }
}

#include "dolphinnewmenu.moc"
