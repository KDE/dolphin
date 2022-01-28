/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINPLACESMODELSINGLETON_H
#define DOLPHINPLACESMODELSINGLETON_H

#include <QString>
#include <QScopedPointer>

#include <KFilePlacesModel>

/**
 * @brief Dolphin's special-cased KFilePlacesModel
 *
 * It returns the trash's icon based on whether
 * it is full or not.
 */
class DolphinPlacesModel : public KFilePlacesModel
{
    Q_OBJECT

public:
    explicit DolphinPlacesModel(const QString &alternativeApplicationName, QObject *parent = nullptr);
    ~DolphinPlacesModel() override;

protected:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private Q_SLOTS:
    void slotTrashEmptinessChanged(bool isEmpty);

private:
    bool isTrash(const QModelIndex &index) const;

    bool m_isEmpty = false;
};

/**
 * @brief Provides a global KFilePlacesModel instance.
 */
class DolphinPlacesModelSingleton
{

public:
    static DolphinPlacesModelSingleton& instance();

    DolphinPlacesModel *placesModel() const;
    /** A suffix to the application-name of the stored bookmarks is
     added, which is only read by PlacesItemModel. */
    static QString applicationNameSuffix();

    DolphinPlacesModelSingleton(const DolphinPlacesModelSingleton&) = delete;
    DolphinPlacesModelSingleton& operator=(const DolphinPlacesModelSingleton&) = delete;

private:
    DolphinPlacesModelSingleton();

    QScopedPointer<DolphinPlacesModel> m_placesModel;
};

#endif // DOLPHINPLACESMODELSINGLETON_H
