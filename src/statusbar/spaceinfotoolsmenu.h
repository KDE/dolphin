/***************************************************************************
 *   Copyright (C) 2014 by Gregor Mi <codestruct@posteo.org>               *
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

#ifndef SPACEINFOTOOLSMENU_H
#define SPACEINFOTOOLSMENU_H

#include <QObject>
#include <QMenu>

class QWidget;
class QUrl;

/**
 * A menu with tools that help to find out more about free disk space for the given url.
 */
class SpaceInfoToolsMenu : public QMenu
{
    Q_OBJECT

public:
    explicit SpaceInfoToolsMenu(QWidget* parent, QUrl url);
    virtual ~SpaceInfoToolsMenu();
};

#endif
