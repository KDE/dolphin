/*
 * SPDX-FileCopyrightText: 2013 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KDIRECTORYCONTENTSCOUNTERWORKER_H
#define KDIRECTORYCONTENTSCOUNTERWORKER_H

#include <QMetaType>
#include <QObject>

class QString;

class KDirectoryContentsCounterWorker : public QObject
{
    Q_OBJECT

public:
    enum Option { NoOptions = 0x0, CountHiddenFiles = 0x1 };
    Q_DECLARE_FLAGS(Options, Option)

    explicit KDirectoryContentsCounterWorker(QObject *parent = nullptr);

    bool stopping() const;
    QString scannedPath() const;
Q_SIGNALS:
    /**
     * Signals that the directory \a path contains \a count items and optionally the size of its content.
     */
    void result(const QString &path, int count, long long size);
    void intermediateResult(const QString &path, int count, long long size);
    void finished();

public Q_SLOTS:
    /**
     * Requests the number of items inside the directory \a path using the
     * options \a options. The result is announced via the signal \a result.
     */
    // Note that the full type name KDirectoryContentsCounterWorker::Options
    // is needed here. Just using 'Options' is OK for the compiler, but
    // confuses moc.
    void countDirectoryContents(const QString &path, KDirectoryContentsCounterWorker::Options options, int maxRecursiveLevel);
    void stop();

private:
#ifndef Q_OS_WIN
    void walkDir(const QString &dirPath, bool countHiddenFiles, uint allowedRecursiveLevel);
#endif

    QString m_scannedPath;

    std::atomic<bool> m_stopping = false;
};

Q_DECLARE_METATYPE(KDirectoryContentsCounterWorker::Options)
Q_DECLARE_OPERATORS_FOR_FLAGS(KDirectoryContentsCounterWorker::Options)

#endif
