/*****************************************************************************
 * Copyright (C) 2009-2010 by Peter Penz <peter.penz@gmx.at>                 *
 * Copyright (C) 2009 by Sebastian Trueg <trueg@kde.org>                     *
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

#include "kloadmetadatathread_p.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kfilemetainfo.h>
#include <kfilemetainfoitem.h>
#include <kglobal.h>
#include <klocale.h>
#include "kmetadatamodel.h"
#include <kprotocolinfo.h>

#include <nepomuk/resource.h>
#include <nepomuk/resourcemanager.h>

KLoadMetaDataThread::KLoadMetaDataThread(KMetaDataModel* model) :
    m_model(model),
    m_data(),
    m_urls(),
    m_canceled(false)
{
    Q_ASSERT(model != 0);
}

KLoadMetaDataThread::~KLoadMetaDataThread()
{
}

void KLoadMetaDataThread::load(const KUrl::List& urls)
{
    m_urls = urls;
    m_canceled = false;
    start();
}

QHash<KUrl, Nepomuk::Variant> KLoadMetaDataThread::data() const
{
    return m_data;
}

void KLoadMetaDataThread::cancel()
{
    // Setting m_canceled to true will cancel KLoadMetaDataThread::run()
    // as soon as run() gets the chance to check m_cancel.
    m_canceled = true;
}

void KLoadMetaDataThread::cancelAndDelete()
{
    if (isFinished()) {
        Q_ASSERT(!isRunning());
        deleteLater();
    } else {
        connect(this, SIGNAL(finished()), this, SLOT(slotFinished()));
        // Setting m_canceled to true will cancel KLoadMetaDataThread::run()
        // as soon as run() gets the chance to check m_cancel.
        m_canceled = true;
        // Afterwards the thread will delete itself
        // asynchronously inside slotFinished().
    }
}

void KLoadMetaDataThread::run()
{
    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");

    unsigned int rating = 0;
    QString comment;
    QList<Nepomuk::Tag> tags;

    bool first = true;
    foreach (const KUrl& url, m_urls) {
        if (m_canceled) {
            return;
        }

        Nepomuk::Resource file(url);
        if (!file.isValid()) {
            continue;
        }

        if (!first && (rating != file.rating())) {
            rating = 0; // reset rating
        } else if (first) {
            rating = file.rating();
        }

        if (!first && (comment != file.description())) {
            comment.clear(); // reset comment
        } else if (first) {
            comment = file.description();
        }

        if (!first && (tags != file.tags())) {
            tags.clear(); // reset tags
        } else if (first) {
            tags = file.tags();
        }

        if (first && (m_urls.count() == 1)) {
            // get cached meta data by checking the indexed files
            QHash<QUrl, Nepomuk::Variant> variants = file.properties();
            QHash<QUrl, Nepomuk::Variant>::const_iterator it = variants.constBegin();
            while (it != variants.constEnd()) {
                Nepomuk::Types::Property prop(it.key());
                const QString uriString = prop.uri().toString();
                if (settings.readEntry(uriString, true)) {
                    m_data.insert(uriString, formatValue(it.value()));
                }
                ++it;
            }

            if (variants.isEmpty()) {
                // the file has not been indexed, query the meta data
                // directly from the file
                KFileMetaInfo metaInfo(m_urls.first());
                const QHash<QString, KFileMetaInfoItem> metaInfoItems = metaInfo.items();
                foreach (const KFileMetaInfoItem& metaInfoItem, metaInfoItems) {
                    const QString uriString = metaInfoItem.name();
                    if (settings.readEntry(uriString, true)) {
                        const Nepomuk::Variant value(metaInfoItem.value());
                        m_data.insert(uriString, formatValue(value));
                    }
                }
            }
        }

        first = false;
    }

    const bool isNepomukActivated = (Nepomuk::ResourceManager::instance()->init() == 0);
    if (isNepomukActivated) {
        m_data.insert(KUrl("kfileitem#rating"), rating);
        m_data.insert(KUrl("kfileitem#comment"), comment);

        QList<Nepomuk::Variant> tagVariants;
        foreach (const Nepomuk::Tag& tag, tags) {
            tagVariants.append(Nepomuk::Variant(tag));
        }
        m_data.insert(KUrl("kfileitem#tags"), tagVariants);
    }

    m_data.unite(m_model->loadData());
}

void KLoadMetaDataThread::slotFinished()
{
    deleteLater();
}

QString  KLoadMetaDataThread::formatValue(const Nepomuk::Variant& value)
{
    if (value.isDateTime()) {
        return KGlobal::locale()->formatDateTime(value.toDateTime(), KLocale::FancyLongDate);
    }

    if (value.isResource() || value.isResourceList()) {
        QStringList links;
        foreach(const Nepomuk::Resource& res, value.toResourceList()) {
            if (KProtocolInfo::isKnownProtocol(res.resourceUri())) {
                links << QString::fromLatin1("<a href=\"%1\">%2</a>")
                         .arg(KUrl(res.resourceUri()).url())
                         .arg(res.genericLabel());
            } else {
                links << res.genericLabel();
            }
        }
        return links.join(QLatin1String(";\n"));
    }

    return value.toString();
}

#include "kloadmetadatathread_p.moc"
