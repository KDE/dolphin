/*****************************************************************************
 * Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                      *
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

#include "nfotranslator.h"

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kfilemetainfo.h>
#include <kfilemetainfoitem.h>
#include <kglobal.h>
#include <klocale.h>
#include <kprotocolinfo.h>

#include <nepomuk/resource.h>

KLoadMetaDataThread::KLoadMetaDataThread() :
    m_rating(0),
    m_comment(),
    m_tags(),
    m_items(),
    m_files(),
    m_urls(),
    m_canceled(false)
{
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

void KLoadMetaDataThread::cancelAndDelete()
{
    connect(this, SIGNAL(finished()), this, SLOT(slotFinished()));
    m_canceled = true;
    // Setting m_canceled to true will cancel KLoadMetaDataThread::run()
    // as soon as possible. Afterwards the thread will delete itself
    // asynchronously inside slotFinished().
}

void KLoadMetaDataThread::run()
{
    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");

    bool first = true;
    foreach (const KUrl& url, m_urls) {
        if (m_canceled) {
            return;
        }

        Nepomuk::Resource file(url);
        if (!file.isValid()) {
            continue;
        }

        m_files.insert(url, file);

        if (!first && (m_rating != file.rating())) {
            m_rating = 0; // reset rating
        } else if (first) {
            m_rating = file.rating();
        }

        if (!first && (m_comment != file.description())) {
            m_comment.clear(); // reset comment
        } else if (first) {
            m_comment = file.description();
        }

        if (!first && (m_tags != file.tags())) {
            m_tags.clear(); // reset tags
        } else if (first) {
            m_tags = file.tags();
        }

        NfoTranslator& nfo = NfoTranslator::instance();
        if (first && (m_urls.count() == 1)) {
            // get cached meta data by checking the indexed files
            QHash<QUrl, Nepomuk::Variant> variants = file.properties();
            QHash<QUrl, Nepomuk::Variant>::const_iterator it = variants.constBegin();
            while (it != variants.constEnd()) {
                Nepomuk::Types::Property prop(it.key());
                if (settings.readEntry(prop.name(), true)) {
                    Item item;
                    item.name = prop.uri().toString();
                    item.label = nfo.translation(prop.uri());
                    item.value = formatValue(it.value());
                    m_items.append(item);
                }
                ++it;
            }

            if (variants.isEmpty()) {
                // the file has not been indexed, query the meta data
                // directly from the file
                KFileMetaInfo metaInfo(m_urls.first());
                const QHash<QString, KFileMetaInfoItem> metaInfoItems = metaInfo.items();
                foreach (const KFileMetaInfoItem& metaInfoItem, metaInfoItems) {
                    Item item;
                    item.name = metaInfoItem.name();
                    item.label = nfo.translation(metaInfoItem.name());
                    item.value = metaInfoItem.value().toString();
                    m_items.append(item);
                }
            }
        }

        first = false;
    }
}

int KLoadMetaDataThread::rating() const
{
    return m_rating;
}

QString KLoadMetaDataThread::comment() const
{
    return m_comment;
}

QList<Nepomuk::Tag> KLoadMetaDataThread::tags() const
{
    return m_tags;
}

QList<KLoadMetaDataThread::Item> KLoadMetaDataThread::items() const
{
    return m_items;
}

QMap<KUrl, Nepomuk::Resource> KLoadMetaDataThread::files() const
{
    return m_files;
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
