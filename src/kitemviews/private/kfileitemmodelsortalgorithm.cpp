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

#include "kfileitemmodelsortalgorithm.h"

void KFileItemModelSortAlgorithm::sort(KFileItemModel* model,
                                       QList<KFileItemModel::ItemData*>::iterator begin,
                                       QList<KFileItemModel::ItemData*>::iterator end)
{
    // The implementation is based on qStableSortHelper() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).

    const int span = end - begin;
    if (span < 2) {
        return;
    }

    const QList<KFileItemModel::ItemData*>::iterator middle = begin + span / 2;
    sort(model, begin, middle);
    sort(model, middle, end);
    merge(model, begin, middle, end);
}

void KFileItemModelSortAlgorithm::merge(KFileItemModel* model,
                                        QList<KFileItemModel::ItemData*>::iterator begin,
                                        QList<KFileItemModel::ItemData*>::iterator pivot,
                                        QList<KFileItemModel::ItemData*>::iterator end)
{
    // The implementation is based on qMerge() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).

    const int len1 = pivot - begin;
    const int len2 = end - pivot;

    if (len1 == 0 || len2 == 0) {
        return;
    }

    if (len1 + len2 == 2) {
        if (model->lessThan(*(begin + 1), *(begin))) {
            qSwap(*begin, *(begin + 1));
        }
        return;
    }

    QList<KFileItemModel::ItemData*>::iterator firstCut;
    QList<KFileItemModel::ItemData*>::iterator secondCut;
    int len2Half;
    if (len1 > len2) {
        const int len1Half = len1 / 2;
        firstCut = begin + len1Half;
        secondCut = lowerBound(model, pivot, end, *firstCut);
        len2Half = secondCut - pivot;
    } else {
        len2Half = len2 / 2;
        secondCut = pivot + len2Half;
        firstCut = upperBound(model, begin, pivot, *secondCut);
    }

    reverse(firstCut, pivot);
    reverse(pivot, secondCut);
    reverse(firstCut, secondCut);

    const QList<KFileItemModel::ItemData*>::iterator newPivot = firstCut + len2Half;
    merge(model, begin, firstCut, newPivot);
    merge(model, newPivot, secondCut, end);
}


QList<KFileItemModel::ItemData*>::iterator
KFileItemModelSortAlgorithm::lowerBound(KFileItemModel* model,
                                        QList<KFileItemModel::ItemData*>::iterator begin,
                                        QList<KFileItemModel::ItemData*>::iterator end,
                                        const KFileItemModel::ItemData* value)
{
    // The implementation is based on qLowerBound() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).

    QList<KFileItemModel::ItemData*>::iterator middle;
    int n = int(end - begin);
    int half;

    while (n > 0) {
        half = n >> 1;
        middle = begin + half;
        if (model->lessThan(*middle, value)) {
            begin = middle + 1;
            n -= half + 1;
        } else {
            n = half;
        }
    }
    return begin;
}

QList<KFileItemModel::ItemData*>::iterator
KFileItemModelSortAlgorithm::upperBound(KFileItemModel* model,
                                        QList<KFileItemModel::ItemData*>::iterator begin,
                                        QList<KFileItemModel::ItemData*>::iterator end,
                                        const KFileItemModel::ItemData* value)
{
    // The implementation is based on qUpperBound() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).

    QList<KFileItemModel::ItemData*>::iterator middle;
    int n = end - begin;
    int half;

    while (n > 0) {
        half = n >> 1;
        middle = begin + half;
        if (model->lessThan(value, *middle)) {
            n = half;
        } else {
            begin = middle + 1;
            n -= half + 1;
        }
    }
    return begin;
}

void KFileItemModelSortAlgorithm::reverse(QList<KFileItemModel::ItemData*>::iterator begin,
                                          QList<KFileItemModel::ItemData*>::iterator end)
{
    // The implementation is based on qReverse() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).

    --end;
    while (begin < end) {
        qSwap(*begin++, *end--);
    }
}
