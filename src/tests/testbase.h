/*****************************************************************************
 *   Copyright (C) 2010-2011 by Frank Reininghaus (frank78ac@googlemail.com) *
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

#ifndef TESTBASE_H
#define TESTBASE_H

#include <QtCore/QObject>

class QAbstractItemView;
class DolphinDirLister;
class DolphinModel;
class DolphinSortFilterProxyModel;
class DolphinView;

/**
 * The class TestBase (which is a friend of DolphinView's) provides access to some
 * parts of DolphinView to the unit tests.
 *
 * TODO: TestBase should also backup the DolphinSettings and restore them later!
 */

class TestBase : public QObject
{
    Q_OBJECT

public:

    TestBase() {};
    ~TestBase() {};

    /** Returns the item view (icons, details, or columns) */
    static QAbstractItemView* itemView(const DolphinView* view);

    /**
     * Waits until the view emits its finishedPathLoading(const KUrl&) signal.
     * Asserts if the signal is not received within the given number of milliseconds.
     */
    static void waitForFinishedPathLoading(DolphinView* view, int milliseconds=20000);

    /** Reloads the view and waits for the finishedPathLoading(const KUrl&) signal. */
    static void reloadViewAndWait(DolphinView* view);

    /** Returns the items shown in the view. The order corresponds to the sort order of the view. */
    static QStringList viewItems(const DolphinView* view);

    /** Returns the items which are selected in the view. The order corresponds to the sort order of the view. */
    static QStringList selectedItems(const DolphinView* view);

};

#endif
