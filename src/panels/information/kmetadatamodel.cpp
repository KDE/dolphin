/*****************************************************************************
 * Copyright (C) 2010 by Peter Penz <peter.penz@gmx.at>                      *
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

#include "kmetadatamodel.h"

#include <kfileitem.h>
#include "kloadmetadatathread_p.h"
#include <kurl.h>

class KMetaDataModel::Private
{

public:
    Private(KMetaDataModel* parent);
    ~Private();

    void slotLoadingFinished();

    QList<KFileItem> m_fileItems;
#ifdef HAVE_NEPOMUK
    QHash<KUrl, Nepomuk::Variant> m_data;

    QList<KLoadMetaDataThread*> m_metaDataThreads;
    KLoadMetaDataThread* m_latestMetaDataThread;
#endif

private:
    KMetaDataModel* const q;
};

KMetaDataModel::Private::Private(KMetaDataModel* parent) :
    m_fileItems(),
#ifdef HAVE_NEPOMUK
    m_data(),
    m_metaDataThreads(),
    m_latestMetaDataThread(0),
#endif
    q(parent)
{
}

KMetaDataModel::Private::~Private()
{
}

KMetaDataModel::KMetaDataModel(QObject* parent) :
    QObject(parent),
    d(new Private(this))
{
}

KMetaDataModel::~KMetaDataModel()
{
    delete d;
}

void KMetaDataModel::Private::slotLoadingFinished()
{
#ifdef HAVE_NEPOMUK
    // The thread that has emitted the finished() signal
    // will get deleted and removed from m_metaDataThreads.
    const int threadsCount = m_metaDataThreads.count();
    for (int i = 0; i < threadsCount; ++i) {
        KLoadMetaDataThread* thread = m_metaDataThreads[i];
        if (thread == q->sender()) {
            m_metaDataThreads.removeAt(i);
            if (thread != m_latestMetaDataThread) {
                // Ignore data of older threads, as the data got
                // obsolete by m_latestMetaDataThread.
                thread->deleteLater();
                return;
            }
        }
    }

    m_data = m_latestMetaDataThread->data();
    m_latestMetaDataThread->deleteLater();
#endif

    emit q->loadingFinished();
}

void KMetaDataModel::setItems(const KFileItemList& items)
{
    d->m_fileItems = items;

#ifdef HAVE_NEPOMUK
    QList<KUrl> urls;
    foreach (const KFileItem& item, items) {
        const KUrl url = item.nepomukUri();
        if (url.isValid()) {
            urls.append(url);
        }
    }

    // Cancel all threads that have not emitted a finished() signal.
    // The deleting of those threads is done in slotLoadingFinished().
    foreach (KLoadMetaDataThread* thread, d->m_metaDataThreads) {
        thread->cancel();
    }

    // create a new thread that will provide the meeta data for the items
    d->m_latestMetaDataThread = new KLoadMetaDataThread(this);
    connect(d->m_latestMetaDataThread, SIGNAL(finished()), this, SLOT(slotLoadingFinished()));
    d->m_latestMetaDataThread->load(urls);
    d->m_metaDataThreads.append(d->m_latestMetaDataThread);
#endif
}

QString KMetaDataModel::group(const KUrl& metaDataUri) const
{
    QString group; // return value

    const QString uri = metaDataUri.url();
    if (uri == QLatin1String("http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#width")) {
        group = QLatin1String("0sizeA");
    } else if (uri == QLatin1String("http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#height")) {
        group = QLatin1String("0sizeB");
    }

    return group;
}

KFileItemList KMetaDataModel::items() const
{
    return d->m_fileItems;
}

#ifdef HAVE_NEPOMUK
QHash<KUrl, Nepomuk::Variant> KMetaDataModel::data() const
{
    return d->m_data;
}

QHash<KUrl, Nepomuk::Variant> KMetaDataModel::loadData() const
{
    return QHash<KUrl, Nepomuk::Variant>();
}
#endif

#include "kmetadatamodel.moc"
