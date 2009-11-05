/*****************************************************************************
 * Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                      *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#ifndef KLOADMETADATATHREAD_H
#define KLOADMETADATATHREAD_H

#define DISABLE_NEPOMUK_LEGACY
#include <nepomuk/property.h>
#include <nepomuk/tag.h>

#include <kurl.h>
#include <QList>
#include <QThread>

/**
 * Loads the meta data of files that are
 * required by the widget KMetaDataWidget.
 */
class KLoadMetaDataThread : public QThread
{
    Q_OBJECT

public:
    KLoadMetaDataThread();
    virtual ~KLoadMetaDataThread();

    /**
     * Starts the thread and loads the meta data for
     * the files given by \p urls. After receiving
     * the signal finished(), the methods
     * rating(), comment(), tags(), metaInfoLabels(),
     * metaInfoValues() and files() return valid data.
     */
    void load(const KUrl::List& urls);

    /**
     * Cancels the thread and assures that the thread deletes
     * itself as soon as the cancelling has been successful. In
     * opposite to QThread::wait() the caller of cancelAndDelete()
     * will not be blocked.
     */
    void cancelAndDelete();

    /** @see QThread::run() */
    virtual void run();

    int rating() const;
    QString comment() const;
    QList<Nepomuk::Tag> tags() const;
    QList<QString> metaInfoLabels() const;
    QList<QString> metaInfoValues() const;
    QMap<KUrl, Nepomuk::Resource> files() const;

private slots:
    void slotFinished();

private:
    /**
     * Assures that the settings for the meta information
     * are initialized with proper default values.
     */
    void initMetaInfoSettings(KConfigGroup& group);

    /**
     * Temporary helper method for KDE 4.3 as we currently don't get
     * translated labels for Nepmok literals: Replaces camelcase labels
     * like "fileLocation" by "File Location:".
     */
    QString tunedLabel(const QString& label) const;

    /**
     * Temporary helper method which tries to pretty print
     * values.
     */
    QString formatValue(const Nepomuk::Variant& value);

private:
    int m_rating;
    QString m_comment;
    QList<Nepomuk::Tag> m_tags;
    QList<QString> m_metaInfoLabels;
    QList<QString> m_metaInfoValues;
    QMap<KUrl, Nepomuk::Resource> m_files;

    KUrl::List m_urls;
    bool m_canceled;
};
#endif
