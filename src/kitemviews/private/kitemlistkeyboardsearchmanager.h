/***************************************************************************
 * Copyright (C) 2011 by Tirtha Chatterjee <tirtha.p.chatterjee@gmail.com> *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#ifndef KITEMLISTKEYBOARDSEARCHMANAGER_H
#define KITEMLISTKEYBOARDSEARCHMANAGER_H

#include <libdolphin_export.h>

#include <QObject>
#include <QString>
#include <QElapsedTimer>

/**
 * @brief Controls the keyboard searching ability for a KItemListController.
 *
 * @see KItemListController
 * @see KItemModelBase
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListKeyboardSearchManager : public QObject
{
    Q_OBJECT

public:

    KItemListKeyboardSearchManager(QObject* parent = 0);
    virtual ~KItemListKeyboardSearchManager();

    /**
     * Add \a keys to the text buffer used for searching.
     */
    void addKeys(const QString& keys);

    /**
     * Sets the delay after which the search is cancelled to \a milliseconds.
     * If the time interval between two calls of addKeys(const QString&) is
     * larger than this, the second call will start a new search, rather than
     * combining the keys received from both calls to a single search string.
     */
    void setTimeout(qint64 milliseconds);
    qint64 timeout() const;

    void cancelSearch();

public slots:

    void slotCurrentChanged(int current, int previous);

signals:
    /**
     * Is emitted if the current item should be changed corresponding
     * to \a text.
     * @param searchFromNextItem If true start searching from item next to the
     *                           current item. Otherwise, search from the
     *                           current item.
     */
    // TODO: Think about getting rid of the bool parameter
    // (see http://doc.qt.nokia.com/qq/qq13-apis.html#thebooleanparametertrap)
    void changeCurrentItem(const QString& string, bool searchFromNextItem);

private:
    QString m_searchedString;
    QElapsedTimer m_keyboardInputTime;
    qint64 m_timeout;
};

#endif


