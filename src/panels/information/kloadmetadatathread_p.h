/*****************************************************************************
 * Copyright (C) 2009-2010 by Peter Penz <peter.penz@gmx.at>                 *
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
#include <nepomuk/variant.h>

#include <kurl.h>
#include <QList>
#include <QThread>

class KMetaDataModel;

/**
 * Loads the meta data of files that are
 * required by the widget KMetaDataWidget.
 */
class KLoadMetaDataThread : public QThread
{
    Q_OBJECT

public:
    KLoadMetaDataThread(KMetaDataModel* model);
    virtual ~KLoadMetaDataThread();

    /**
     * Starts the thread and loads the meta data for
     * the files given by \p urls. After receiving
     * the signal finished(), the method KLoadMetaDataThread::data()
     * provides the loaded meta data.
     */
    void load(const KUrl::List& urls);

    /**
     * Returns the meta data for the URLs given
     * by KLoadMetaDataThread::load(). The method only provides
     * valid results after the signal finished() has been
     * emitted.
     */
    QMap<KUrl, Nepomuk::Variant> data() const;

    /**
     * Tells the thread that it should cancel as soon
     * as possible. It is undefined when the thread
     * gets cancelled. The signal finished() will emitted
     * after the cancelling has been done.mergedIt
     */
    void cancel();

    /**
     * Cancels the thread and assures that the thread deletes
     * itself as soon as the cancelling has been successful. In
     * opposite to QThread::wait() the caller of cancelAndDelete()
     * will not be blocked.
     */
    void cancelAndDelete();

    /** @see QThread::run() */
    virtual void run();

private slots:
    void slotFinished();

private:
    /**
     * Temporary helper method until there is a proper formatting facility in Nepomuk.
     * Here we simply handle the most common formatting situations that do not look nice
     * when using Nepomuk::Variant::toString().
     */
    QString formatValue(const Nepomuk::Variant& value);

private:
    KMetaDataModel* m_model;
    QMap<KUrl, Nepomuk::Variant> m_data;
    KUrl::List m_urls;
    bool m_canceled;
};
#endif
