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

#include "dolphinquery.h"

#include <config-baloo.h>
#ifdef HAVE_BALOO
#include <Baloo/Query>
#endif

namespace {
    /** Checks if a given term in the Baloo::Query::searchString() is a special search term.
     * This is a copy of `DolphinFacetsWidget::isRatingTerm()` method.
     */
    bool isSearchTerm(const QString& term)
    {
        static const QLatin1String searchTokens[] {
            QLatin1String("modified>="),
            QLatin1String("rating>=")
        };

        for (const auto &searchToken : searchTokens) {
            if (term.startsWith(searchToken)) {
                return true;
            }
        }
        return false;
    }
}

DolphinQuery DolphinQuery::fromBalooSearchUrl(const QUrl& searchUrl)
{
    DolphinQuery model;
    model.m_searchUrl = searchUrl;

#ifdef HAVE_BALOO
    const Baloo::Query query = Baloo::Query::fromSearchUrl(searchUrl);

    model.m_includeFolder = query.includeFolder();

    model.m_searchText = query.searchString();

    const QStringList types = query.types();
    model.m_fileType = types.isEmpty() ? QString() : types.first();

    const QStringList subTerms = query.searchString().split(' ', QString::SkipEmptyParts);
    foreach (const QString& subTerm, subTerms) {
        if (subTerm.startsWith(QLatin1String("filename:"))) {
            const QString value = subTerm.mid(9);
            model.m_searchText = value;
        } else if (isSearchTerm(subTerm)) {
            model.m_searchTerms << subTerm;
        }
    }
#endif
    return model;
}

QUrl DolphinQuery::searchUrl() const
{
    return m_searchUrl;
}

QString DolphinQuery::text() const
{
    return m_searchText;
}

QString DolphinQuery::type() const
{
    return m_fileType;
}

QStringList DolphinQuery::searchTerms() const
{
    return m_searchTerms;
}

QString DolphinQuery::includeFolder() const
{
    return m_includeFolder;
}
