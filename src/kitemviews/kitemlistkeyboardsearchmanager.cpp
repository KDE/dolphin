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

#include "kitemlistkeyboardsearchmanager_p.h"

#include <QApplication>
#include <QElapsedTimer>

#include <KDebug>

KItemListKeyboardSearchManager::KItemListKeyboardSearchManager(QObject* parent) :
    QObject(parent),
    m_timeout(1000)
{
    m_keyboardInputTime.invalidate();
}

KItemListKeyboardSearchManager::~KItemListKeyboardSearchManager()
{
}

void KItemListKeyboardSearchManager::addKeys(const QString& keys)
{
    const bool keyboardTimeWasValid = m_keyboardInputTime.isValid();
    const qint64 keyboardInputTimeElapsed = m_keyboardInputTime.restart();
    if (keyboardInputTimeElapsed > m_timeout || !keyboardTimeWasValid || keys.isEmpty()) {
        m_searchedString.clear();
    }

    const bool newSearch = m_searchedString.isEmpty();

    if (!keys.isEmpty()) {
        m_searchedString.append(keys);

        // Special case:
        // If the same key is pressed repeatedly, the next item matching that key should be highlighted
        const QChar firstKey = m_searchedString.length() > 0 ? m_searchedString.at(0) : QChar();
        const bool sameKey = m_searchedString.length() > 1 && m_searchedString.count(firstKey) == m_searchedString.length();

        // Searching for a matching item should start from the next item if either
        // 1. a new search is started, or
        // 2. a 'repeated key' search is done.
        const bool searchFromNextItem = newSearch || sameKey;

        emit changeCurrentItem(sameKey ? firstKey : m_searchedString, searchFromNextItem);
    }
    m_keyboardInputTime.start();
}

void KItemListKeyboardSearchManager::setTimeout(qint64 milliseconds)
{
    m_timeout = milliseconds;
}

qint64 KItemListKeyboardSearchManager::timeout() const
{
    return m_timeout;
}

