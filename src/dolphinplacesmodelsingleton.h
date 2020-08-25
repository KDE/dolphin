/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
