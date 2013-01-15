/*****************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>               *
 *   Copyright (C) 2012 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
 *   Copyright (C) 2013 by Frank Reininghaus <frank78ac@googlemail.com>      *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, write to the                           *
 *   Free Software Foundation, Inc.,                                         *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA              *
 *****************************************************************************/

#ifndef KFILEITEMMODELSORTALGORITHM_H
#define KFILEITEMMODELSORTALGORITHM_H

#include <QtCore>

#include <algorithm>

/**
 * Sorts the items using the merge sort algorithm is used to assure a
 * worst-case of O(n * log(n)) and to keep the number of comparisons low.
 *
 * The implementation is based on qStableSortHelper() from qalgorithms.h
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 * The sorting implementations of qAlgorithms could not be used as they
 * don't support having a member-function as comparison criteria.
 */

template <typename RandomAccessIterator, typename LessThan>
static void mergeSort(RandomAccessIterator begin,
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
    mergeSort(begin, middle, lessThan);
    mergeSort(middle, end, lessThan);
    merge(begin, middle, end, lessThan);
}

/**
 * Uses up to \a numberOfThreads threads to sort the items between
 * \a begin and \a end. Only item ranges longer than
 * \a parallelMergeSortingThreshold are split to be sorted by two different
 * threads.
 *
 * The comparison function \a lessThan must be reentrant.
 */

template <typename RandomAccessIterator, typename LessThan>
static void parallelMergeSort(RandomAccessIterator begin,
                              RandomAccessIterator end,
                              LessThan lessThan,
                              int numberOfThreads,
                              int parallelMergeSortingThreshold = 100)
{
    const int span = end - begin;

    if (numberOfThreads > 1 && span > parallelMergeSortingThreshold) {
        const int newNumberOfThreads = numberOfThreads / 2;
        const RandomAccessIterator middle = begin + span / 2;

        QFuture<void> future = QtConcurrent::run(parallelMergeSort<RandomAccessIterator, LessThan>, begin, middle, lessThan, newNumberOfThreads, parallelMergeSortingThreshold);
        parallelMergeSort(middle, end, lessThan, newNumberOfThreads, parallelMergeSortingThreshold);

        future.waitForFinished();

        merge(begin, middle, end, lessThan);
    } else {
        mergeSort(begin, end, lessThan);
    }
}

/**
 * Merges the sorted item ranges between \a begin and \a pivot and
 * between \a pivot and \a end into a single sorted range between
 * \a begin and \a end.
 *
 * The implementation is based on qMerge() from qalgorithms.h
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 * The sorting implementations of qAlgorithms could not be used as they
 * don't support having a member-function as comparison criteria.
 */

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

#endif

