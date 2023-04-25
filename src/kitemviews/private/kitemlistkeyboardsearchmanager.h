/*
 * SPDX-FileCopyrightText: 2011 Tirtha Chatterjee <tirtha.p.chatterjee@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTKEYBOARDSEARCHMANAGER_H
#define KITEMLISTKEYBOARDSEARCHMANAGER_H

#include "dolphin_export.h"
#include "kitemviews/kitemset.h"

#include <QElapsedTimer>
#include <QObject>
#include <QString>

/**
 * @brief Controls the keyboard searching ability for a KItemListController.
 *
 * @see KItemListController
 * @see KItemModelBase
 */
class DOLPHIN_EXPORT KItemListKeyboardSearchManager : public QObject
{
    Q_OBJECT

public:
    explicit KItemListKeyboardSearchManager(QObject *parent = nullptr);
    ~KItemListKeyboardSearchManager() override;

    /**
     * Add \a keys to the text buffer used for searching.
     */
    void addKeys(const QString &keys);

    /**
     * Sets the delay after which the search is cancelled to \a milliseconds.
     * If the time interval between two calls of addKeys(const QString&) is
     * larger than this, the second call will start a new search, rather than
     * combining the keys received from both calls to a single search string.
     */
    void setTimeout(qint64 milliseconds);
    qint64 timeout() const;

    void cancelSearch();

    /**
     * @return \c true if search as you type is active, or \c false otherwise.
     */
    bool isSearchAsYouTypeActive() const;

public Q_SLOTS:

    void slotCurrentChanged(int current, int previous);
    void slotSelectionChanged(const KItemSet &current, const KItemSet &previous);

Q_SIGNALS:
    /**
     * Is emitted if the current item should be changed corresponding
     * to \a text.
     * @param searchFromNextItem If true start searching from item next to the
     *                           current item. Otherwise, search from the
     *                           current item.
     */
    // TODO: Think about getting rid of the bool parameter
    // (see https://doc.qt.io/archives/qq/qq13-apis.html#thebooleanparametertrap)
    void changeCurrentItem(const QString &string, bool searchFromNextItem);

private:
    bool shouldClearSearchIfInputTimeReached();

    QString m_searchedString;
    bool m_isSearchRestarted;
    /** Measures the time since the last key press. */
    QElapsedTimer m_keyboardInputTime;
    /** Time in milliseconds in which a key press is considered as a continuation of the previous search input. */
    qint64 m_timeout;
};

#endif
