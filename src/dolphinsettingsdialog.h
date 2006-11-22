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

#ifndef DOLPHINSETTINGSDIALOG_H
#define DOLPHINSETTINGSDIALOG_H

#include <kpagedialog.h>
class GeneralSettingsPage;
class ViewSettingsPage;
class BookmarksSettingsPage;

/**
 * @brief Settings dialog for Dolphin.
 *
 * Contains the pages for general settings, view settings and
 * bookmark settings.
 *
 * @author Peter Penz <peter.penz@gmx.at>
 */
class DolphinSettingsDialog : public KPageDialog {
    Q_OBJECT

public:
    DolphinSettingsDialog();
    virtual ~DolphinSettingsDialog();

protected slots:
    virtual void slotButtonClicked(int button);

private:
    GeneralSettingsPage* m_generalSettingsPage;
    ViewSettingsPage* m_viewSettingsPage;
    BookmarksSettingsPage* m_bookmarksSettingsPage;

    void applySettings();
};

#endif
