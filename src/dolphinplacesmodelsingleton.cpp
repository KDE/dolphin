/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinplacesmodelsingleton.h"
#include "trash/dolphintrash.h"

#include <KAboutData>
#include <KFilePlacesModel>

#include <QIcon>

DolphinPlacesModel::DolphinPlacesModel(const QString &alternativeApplicationName, QObject *parent)
    : KFilePlacesModel(alternativeApplicationName, parent)
{
    connect(&Trash::instance(), &Trash::emptinessChanged, this, &DolphinPlacesModel::slotTrashEmptinessChanged);
}

DolphinPlacesModel::~DolphinPlacesModel() = default;

QVariant DolphinPlacesModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole) {
        if (isTrash(index)) {
            if (m_isEmpty) {
                return QIcon::fromTheme(QStringLiteral("user-trash"));
            } else {
                return QIcon::fromTheme(QStringLiteral("user-trash-full"));
            }
        }
    }

    return KFilePlacesModel::data(index, role);
}

void DolphinPlacesModel::slotTrashEmptinessChanged(bool isEmpty)
{
    if (m_isEmpty == isEmpty) {
        return;
    }

    // NOTE Trash::isEmpty() reads the config file whereas emptinessChanged is
    // hooked up to whether a dirlister in trash:/ has any files and they disagree...
    m_isEmpty = isEmpty;

    for (int i = 0; i < rowCount(); ++i) {
        const QModelIndex index = this->index(i, 0);
        if (isTrash(index)) {
            Q_EMIT dataChanged(index, index, {Qt::DecorationRole});
        }
    }
}

bool DolphinPlacesModel::isTrash(const QModelIndex &index) const
{
    return url(index) == QUrl(QStringLiteral("trash:/"));
}

DolphinPlacesModelSingleton::DolphinPlacesModelSingleton()
    : m_placesModel(new DolphinPlacesModel(KAboutData::applicationData().componentName() + applicationNameSuffix()))
{

}

DolphinPlacesModelSingleton &DolphinPlacesModelSingleton::instance()
{
    static DolphinPlacesModelSingleton s_self;
    return s_self;
}

DolphinPlacesModel *DolphinPlacesModelSingleton::placesModel() const
{
    return m_placesModel.data();
}

QString DolphinPlacesModelSingleton::applicationNameSuffix()
{
    return QStringLiteral("-places-panel");
}
