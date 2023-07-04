/*
 * SPDX-FileCopyrightText: 2011 Tirtha Chatterjee <tirtha.p.chatterjee@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistkeyboardsearchmanager.h"

KItemListKeyboardSearchManager::KItemListKeyboardSearchManager(QObject *parent)
    : QObject(parent)
    , m_isSearchRestarted(false)
    , m_timeout(1000)
{
    m_keyboardInputTime.invalidate();
}

KItemListKeyboardSearchManager::~KItemListKeyboardSearchManager()
{
}

bool KItemListKeyboardSearchManager::shouldClearSearchIfInputTimeReached()
{
    const bool keyboardTimeWasValid = m_keyboardInputTime.isValid();
    const qint64 keyboardInputTimeElapsed = m_keyboardInputTime.restart();
    return (keyboardInputTimeElapsed > m_timeout) || !keyboardTimeWasValid;
}

bool KItemListKeyboardSearchManager::isSearchAsYouTypeActive() const
{
    return !m_searchedString.isEmpty() && !m_keyboardInputTime.hasExpired(m_timeout);
}

void KItemListKeyboardSearchManager::addKeys(const QString &keys)
{
    if (shouldClearSearchIfInputTimeReached()) {
        m_searchedString.clear();
    }

    const bool newSearch = m_searchedString.isEmpty();

    // Do not start a new search if the user pressed Space. Only add
    // it to the search string if a search is in progress already.
    if (newSearch && keys == QLatin1Char(' ')) {
        return;
    }

    if (!keys.isEmpty()) {
        m_searchedString.append(keys);

        // Special case:
        // If the same key is pressed repeatedly, the next item matching that key should be highlighted
        const QChar firstKey = m_searchedString.length() > 0 ? m_searchedString.at(0) : QChar();
        const bool sameKey = m_searchedString.length() > 1 && m_searchedString.count(firstKey) == m_searchedString.length();

        // Searching for a matching item should start from the next item if either
        // 1. a new search is started and a search has not been restarted or
        // 2. a 'repeated key' search is done.
        const bool searchFromNextItem = (!m_isSearchRestarted && newSearch) || sameKey;

        // to remember not to searchFromNextItem if selection was deselected
        // losing keyboard search context basically
        m_isSearchRestarted = false;

        Q_EMIT changeCurrentItem(sameKey ? firstKey : m_searchedString, searchFromNextItem);
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

void KItemListKeyboardSearchManager::cancelSearch()
{
    m_isSearchRestarted = true;
    m_searchedString.clear();
}

void KItemListKeyboardSearchManager::slotCurrentChanged(int current, int previous)
{
    Q_UNUSED(previous)

    if (current < 0) {
        // The current item has been removed. We should cancel the search.
        cancelSearch();
    }
}

void KItemListKeyboardSearchManager::slotSelectionChanged(const KItemSet &current, const KItemSet &previous)
{
    if (!previous.isEmpty() && current.isEmpty() && previous.count() > 0 && current.count() == 0) {
        // The selection has been emptied. We should cancel the search.
        cancelSearch();
    }
}

#include "moc_kitemlistkeyboardsearchmanager.cpp"
