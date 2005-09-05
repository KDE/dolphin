/*
 * Copyright (c) 2000 Malte Starostik <malte@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <ktrader.h>

#include "searchprovider.h"

SearchProvider::SearchProvider(const KService::Ptr service)
               : m_dirty(false)
{
    m_desktopEntryName = service->desktopEntryName();
    m_name = service->name();
    m_query = service->property("Query").toString();
    m_keys = service->property("Keys").toStringList();
    m_charset = service->property("Charset").toString();
}

void SearchProvider::setName(const QString &name)
{
    if (m_name == name)
        return;
    m_name = name;
    m_dirty = true;
}

void SearchProvider::setQuery(const QString &query)
{
    if (m_query == query)
        return;
    m_query = query;
    m_dirty = true;
}

void SearchProvider::setKeys(const QStringList &keys)
{
    if (m_keys == keys)
        return;
    m_keys = keys;
    m_dirty = true;
}

void SearchProvider::setCharset(const QString &charset)
{
    if (m_charset == charset)
        return;
    m_charset = charset;
    m_dirty = true;
}

SearchProvider *SearchProvider::findByDesktopName(const QString &name)
{
    KService::Ptr service =
        KService::serviceByDesktopPath(QString("searchproviders/%1.desktop").arg(name));
    return service ? new SearchProvider(service) : 0;
}

SearchProvider *SearchProvider::findByKey(const QString &key)
{
    KTrader::OfferList providers =
        KTrader::self()->query("SearchProvider", QString("'%1' in Keys").arg(key));
    return providers.count() ? new SearchProvider(providers[0]) : 0;
}

