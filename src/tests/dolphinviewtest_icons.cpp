/***************************************************************************
 *   Copyright (C) 2010 by Frank Reininghaus (frank78ac@googlemail.com)    *
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

#include <qtest_kde.h>

#include "dolphinviewtest_allviewmodes.h"

class DolphinViewTest_Icons : public DolphinViewTest_AllViewModes
{
    Q_OBJECT

public:

    virtual DolphinView::Mode mode() const {
        return DolphinView::IconsView;
    }

    virtual bool verifyCorrectViewMode() const {
        return (m_view->mode() == DolphinView::IconsView);
    }

};

QTEST_KDEMAIN(DolphinViewTest_Icons, GUI)

#include "dolphinviewtest_icons.moc"
