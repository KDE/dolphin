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

#include <QThread>
#include <QtCore>

#include <algorithm>

class KFileItemModelLessThan
{
public:
    KFileItemModelLessThan(KFileItemModel* model) :
        m_model(model)
    {
    }

    bool operator()(const KFileItemModel::ItemData* a, const KFileItemModel::ItemData* b)
    {
        return m_model->lessThan(a, b);
    }

private:
    KFileItemModel* m_model;
};

void KFileItemModelSortAlgorithm::sort(KFileItemModel* model,
                                       QList<KFileItemModel::ItemData*>::iterator begin,
                                       QList<KFileItemModel::ItemData*>::iterator end)
{
    KFileItemModelLessThan lessThan(model);

    if (model->sortRole() == model->roleForType(KFileItemModel::NameRole)) {
        // Sorting by name can be expensive, in particular if natural sorting is
        // enabled. Use all CPU cores to speed up the sorting process.
        static const int numberOfThreads = QThread::idealThreadCount();
        parallelSort(begin, end, lessThan, numberOfThreads);
    } else {
        // Sorting by other roles is quite fast. Use only one thread to prevent
        // problems caused by non-reentrant comparison functions, see
        // https://bugs.kde.org/show_bug.cgi?id=312679
        sequentialSort(begin, end, lessThan);
    }
}

template <typename RandomAccessIterator, typename LessThan>
static void sequentialSort(RandomAccessIterator begin,
                           RandomAccessIterator end,
                           LessThan lessThan)
{
    // The implementation is based on qStableSortHelper() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).

    const int span = end - begin;
    if (span < 2) {
        return;
    }

    const RandomAccessIterator middle = begin + span / 2;
    sequentialSort(begin, middle, lessThan);
    sequentialSort(middle, end, lessThan);
    merge(begin, middle, end, lessThan);
}

template <typename RandomAccessIterator, typename LessThan>
static void parallelSort(RandomAccessIterator begin,
                         RandomAccessIterator end,
                         LessThan lessThan,
                         int numberOfThreads,
                         int parallelSortingThreshold)
{
    const int span = end - begin;

    if (numberOfThreads > 1 && span > parallelSortingThreshold) {
        const int newNumberOfThreads = numberOfThreads / 2;
        const RandomAccessIterator middle = begin + span / 2;

        QFuture<void> future = QtConcurrent::run(parallelSort<RandomAccessIterator, LessThan>, begin, middle, lessThan, newNumberOfThreads, parallelSortingThreshold);
        parallelSort(middle, end, lessThan, newNumberOfThreads, parallelSortingThreshold);

        future.waitForFinished();

        merge(begin, middle, end, lessThan);
    } else {
        sequentialSort(begin, end, lessThan);
    }
}

template <typename RandomAccessIterator, typename LessThan>
static void merge(RandomAccessIterator begin,
                  RandomAccessIterator pivot,
                  RandomAccessIterator end,
                  LessThan lessThan)
{
    // The implementation is based on qMerge() from qalgorithms.h
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).

    const int len1 = pivot - begin;
    const int len2 = end - pivot;

    if (len1 == 0 || len2 == 0) {
        return;
    }

    if (len1 + len2 == 2) {
        if (lessThan(*(begin + 1), *(begin))) {
            qSwap(*begin, *(begin + 1));
        }
        return;
    }

    RandomAccessIterator firstCut;
    RandomAccessIterator secondCut;
    int len2Half;
    if (len1 > len2) {
        const int len1Half = len1 / 2;
        firstCut = begin + len1Half;
        secondCut = std::lower_bound(pivot, end, *firstCut, lessThan);
        len2Half = secondCut - pivot;
    } else {
        len2Half = len2 / 2;
        secondCut = pivot + len2Half;
        firstCut = std::upper_bound(begin, pivot, *secondCut, lessThan);
    }

    std::rotate(firstCut, pivot, secondCut);

    RandomAccessIterator newPivot = firstCut + len2Half;
    merge(begin, firstCut, newPivot, lessThan);
    merge(newPivot, secondCut, end, lessThan);
}
