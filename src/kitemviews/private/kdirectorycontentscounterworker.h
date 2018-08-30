/***************************************************************************
 *   Copyright (C) 2013 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#ifndef KDIRECTORYCONTENTSCOUNTERWORKER_H
#define KDIRECTORYCONTENTSCOUNTERWORKER_H

#include <QMetaType>
#include <QObject>

class QString;

class KDirectoryContentsCounterWorker : public QObject
{
    Q_OBJECT

public:
    enum Option {
        NoOptions = 0x0,
        CountHiddenFiles = 0x1,
        CountDirectoriesOnly = 0x2
    };
    Q_DECLARE_FLAGS(Options, Option)

    explicit KDirectoryContentsCounterWorker(QObject* parent = nullptr);

    /**
     * Counts the items inside the directory \a path using the options
     * \a options.
     *
     * @return The number of items.
     */
    static int subItemsCount(const QString& path, Options options);

signals:
    /**
     * Signals that the directory \a path contains \a count items.
     */
    void result(const QString& path, int count);

public slots:
    /**
     * Requests the number of items inside the directory \a path using the
     * options \a options. The result is announced via the signal \a result.
     */
    // Note that the full type name KDirectoryContentsCounterWorker::Options
    // is needed here. Just using 'Options' is OK for the compiler, but
    // confuses moc.
    void countDirectoryContents(const QString& path, KDirectoryContentsCounterWorker::Options options);
};

Q_DECLARE_METATYPE(KDirectoryContentsCounterWorker::Options)
Q_DECLARE_OPERATORS_FOR_FLAGS(KDirectoryContentsCounterWorker::Options)

#endif
