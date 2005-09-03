/*
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef __FILTEROPTS_H___
#define __FILTEROPTS_H___

#include <kcmodule.h>

class QListBox;

class FilterOptions : public KCModule
{
    Q_OBJECT

public:
    FilterOptions(QWidget *parent = 0, const char *name = 0);

    void load();
    void save();
    void defaults();

protected slots:
    void moduleChanged(bool state);

private:
	QListBox* m_lbFilterList;
};

#endif

