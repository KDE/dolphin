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
    QObject(parent)
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
    if (keyboardInputTimeElapsed > QApplication::keyboardInputInterval()
        || !keyboardTimeWasValid || keys.isEmpty()) {
        m_searchedString.clear();
    }
    const bool searchFromNextItem = m_searchedString.isEmpty();
    if (!keys.isEmpty()) {
        m_searchedString.append(keys);
        emit requestItemActivation(m_searchedString, searchFromNextItem);
    }
    m_keyboardInputTime.start();
}
