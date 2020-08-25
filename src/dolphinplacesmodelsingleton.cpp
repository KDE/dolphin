/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinplacesmodelsingleton.h"

#include <KAboutData>
#include <KFilePlacesModel>

DolphinPlacesModelSingleton::DolphinPlacesModelSingleton()
    : m_placesModel(new KFilePlacesModel(KAboutData::applicationData().componentName() + applicationNameSuffix()))
{

}

DolphinPlacesModelSingleton &DolphinPlacesModelSingleton::instance()
{
    static DolphinPlacesModelSingleton s_self;
    return s_self;
}

KFilePlacesModel *DolphinPlacesModelSingleton::placesModel() const
{
    return m_placesModel.data();
}

QString DolphinPlacesModelSingleton::applicationNameSuffix()
{
    return QStringLiteral("-places-panel");
}
