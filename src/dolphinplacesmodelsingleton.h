/***************************************************************************
 *   Copyright (C) 2018 Kai Uwe Broulik <kde@privat.broulik.de>            *
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

#ifndef DOLPHINPLACESMODELSINGLETON_H
#define DOLPHINPLACESMODELSINGLETON_H

#include <QString>
#include <QScopedPointer>

class KFilePlacesModel;

/**
 * @brief Provides a global KFilePlacesModel instance.
 */
class DolphinPlacesModelSingleton
{

public:
    static DolphinPlacesModelSingleton& instance();

    KFilePlacesModel *placesModel() const;
    /** A suffix to the application-name of the stored bookmarks is
     added, which is only read by PlacesItemModel. */
    static QString applicationNameSuffix();

    DolphinPlacesModelSingleton(const DolphinPlacesModelSingleton&) = delete;
    DolphinPlacesModelSingleton& operator=(const DolphinPlacesModelSingleton&) = delete;

private:
    DolphinPlacesModelSingleton();

    QScopedPointer<KFilePlacesModel> m_placesModel;
};

#endif // DOLPHINPLACESMODELSINGLETON_H
