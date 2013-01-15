/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KFILEITEMMODELSORTALGORITHM_H
#define KFILEITEMMODELSORTALGORITHM_H

#include <libdolphin_export.h>

#include <kitemviews/kfileitemmodel.h>

/**
 * @brief Sort algorithm for sorting items of KFileItemModel.
 *
 * Sorts the items by using KFileItemModel::lessThan() as comparison criteria.
 * The merge sort algorithm is used to assure a worst-case
 * of O(n * log(n)) and to keep the number of comparisons low.
 *
 * The implementation is based on qStableSortHelper() from qalgorithms.h
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 * The sorting implementations of qAlgorithms could not be used as they
 * don't support having a member-function as comparison criteria.
 */
class LIBDOLPHINPRIVATE_EXPORT KFileItemModelSortAlgorithm
{
public:
    static void sort(KFileItemModel* model,
                     QList<KFileItemModel::ItemData*>::iterator begin,
                     QList<KFileItemModel::ItemData*>::iterator end);

private:
    static void sequentialSort(KFileItemModel* model,
                               QList<KFileItemModel::ItemData*>::iterator begin,
                               QList<KFileItemModel::ItemData*>::iterator end);

    static void parallelSort(KFileItemModel* model,
                             QList<KFileItemModel::ItemData*>::iterator begin,
                             QList<KFileItemModel::ItemData*>::iterator end,
                             const int numberOfThreads);

    static void merge(KFileItemModel* model,
                      QList<KFileItemModel::ItemData*>::iterator begin,
                      QList<KFileItemModel::ItemData*>::iterator pivot,
                      QList<KFileItemModel::ItemData*>::iterator end);

    static QList<KFileItemModel::ItemData*>::iterator
                lowerBound(KFileItemModel* model,
                           QList<KFileItemModel::ItemData*>::iterator begin,
                           QList<KFileItemModel::ItemData*>::iterator end,
                           const KFileItemModel::ItemData* value);

    static QList<KFileItemModel::ItemData*>::iterator
                upperBound(KFileItemModel* model,
                           QList<KFileItemModel::ItemData*>::iterator begin,
                           QList<KFileItemModel::ItemData*>::iterator end,
                           const KFileItemModel::ItemData* value);
};

#endif


