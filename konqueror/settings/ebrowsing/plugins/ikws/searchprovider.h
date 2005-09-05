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


#ifndef __SEARCHPROVIDER_H___
#define __SEARCHPROVIDER_H___

#include <kservice.h>

class SearchProvider 
{
public:
    SearchProvider() : m_dirty(false) {};
    SearchProvider(const KService::Ptr service);

    const QString &desktopEntryName() const { return m_desktopEntryName; }
    const QString &name() const { return m_name; }
    const QString &query() const { return m_query; }
    const QStringList &keys() const { return m_keys; }
    const QString &charset() const { return m_charset; }
    bool isDirty() const { return m_dirty; }

    void setName(const QString &);
    void setQuery(const QString &);
    void setKeys(const QStringList &);
    void setCharset(const QString &);

    static SearchProvider *findByDesktopName(const QString &);
    static SearchProvider *findByKey(const QString &);
private:
    QString m_desktopEntryName;
    QString m_name;
    QString m_query;
    QStringList m_keys;
    QString m_charset;

    bool m_dirty;
};

#endif
