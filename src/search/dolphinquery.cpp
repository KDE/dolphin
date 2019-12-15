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
            QLatin1String("rating>="),
            QLatin1String("tag:"), QLatin1String("tag=")
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

    const QStringList types = query.types();
    model.m_fileType = types.isEmpty() ? QString() : types.first();

    QStringList textParts;

    const QStringList subTerms = query.searchString().split(' ', QString::SkipEmptyParts);
    foreach (const QString& subTerm, subTerms) {
        QString value;
        if (subTerm.startsWith(QLatin1String("filename:"))) {
            value = subTerm.mid(9);
        } else if (isSearchTerm(subTerm)) {
            model.m_searchTerms << subTerm;
            continue;
        } else if (subTerm == QLatin1String("AND") && subTerm != subTerms.at(0) && subTerm != subTerms.back()) {
            continue;
        } else {
            value = subTerm;
        }

        if (!value.isEmpty() && value.at(0) == QLatin1Char('"')) {
            value = value.mid(1);
        }
        if (!value.isEmpty() && value.back() == QLatin1Char('"')) {
            value = value.mid(0, value.size() - 1);
        }
        if (!value.isEmpty()) {
            textParts << value;
        }
    }

    model.m_searchText = textParts.join(QLatin1Char(' '));

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
