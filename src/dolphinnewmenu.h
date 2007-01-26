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

#ifndef DOLPHINNEWMENU_H
#define DOLPHINNEWMENU_H

class KActionCollection;  // TODO: only required temporary because of
                          // missing forward declaration in knewmenu.h
#include <knewmenu.h>

class DolphinMainWindow;
class KJob;

/**
 * @brief Represents the 'Create New...' sub menu for the File menu
 *        and the context menu.
 *
 * The only difference to KNewMenu is the custom error handling.
 * All errors are shown in the status bar of Dolphin
 * instead as modal error dialog with an OK button.
 */
class DolphinNewMenu : public KNewMenu
{
Q_OBJECT

public:
    DolphinNewMenu(DolphinMainWindow* mainWin);
    virtual ~DolphinNewMenu();

protected slots:
    /** @see KNewMenu::slotResult() */
    virtual void slotResult(KJob* job);

private:
    DolphinMainWindow* m_mainWin;
};

#endif
