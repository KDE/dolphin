/***************************************************************************
 *   Copyright (C) 2019 by Ismael Asensio <isma.af@mgmail.com>             *
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

#ifndef DOLPHINQUERY_H
#define DOLPHINQUERY_H

#include "dolphin_export.h"

#include <QString>
#include <QUrl>

/**
 * @brief Simple query model that parses a Baloo search Url and extracts its
 * separate components to be displayed on dolphin search box.
 */
class DolphinQuery
{
public:
    /** Calls Baloo::Query::fromSearchUrl() with the given @p searchUrl
     * and parses the result to extract its separate components */
    static DolphinQuery fromBalooSearchUrl(const QUrl& searchUrl);

    /** @return the \a searchUrl passed to Baloo::Query::fromSearchUrl() */
    QUrl searchUrl() const;
    /** @return the user text part of the query, to be shown in the searchbar */
    QString text() const;
    /** @return the first of Baloo::Query::types(), or an empty string */
    QString type() const;
    /** @return a list of the search terms of the Baloo::Query that act as a filter,
     * such as \"rating>= <i>value<i>\" or \"modified>= <i>date<i>\"*/
    QStringList searchTerms() const;
    /** @return Baloo::Query::includeFolder(), that is, the initial directory
     * for the query or an empty string if its a global search" */
    QString includeFolder() const;
    /** @return whether the query includes search in file content */
    bool hasContentSearch() const;
    /** @return whether the query includes a filter by fileName */
    bool hasFileName() const;

private:
    QUrl m_searchUrl;
    QString m_searchText;
    QString m_fileType;
    QStringList m_searchTerms;
    QString m_includeFolder;
    bool m_hasContentSearch = false;
    bool m_hasFileName = false;
};

#endif //DOLPHINQUERY_H
