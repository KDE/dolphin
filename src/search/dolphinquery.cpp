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

#include <QRegularExpression>

#include <config-baloo.h>
#ifdef HAVE_BALOO
#include <Baloo/Query>
#endif

namespace {
#ifdef HAVE_BALOO
    /** Checks if a given term in the Baloo::Query::searchString() is a special search term
     * @return: the specific search token of the term, or an empty QString() if none is found
     */
    QString searchTermToken(const QString& term)
    {
        static const QLatin1String searchTokens[] {
            QLatin1String("filename:"),
            QLatin1String("modified>="),
            QLatin1String("rating>="),
            QLatin1String("tag:"), QLatin1String("tag=")
        };

        for (const auto &searchToken : searchTokens) {
            if (term.startsWith(searchToken)) {
                return searchToken;
            }
        }
        return QString();
    }

    QString stripQuotes(const QString& text)
    {
        if (text.length() >= 2 && text.at(0) == QLatin1Char('"')
                               && text.back() == QLatin1Char('"')) {
            return text.mid(1, text.size() - 2);
        }
        return text;
    }

    QStringList splitOutsideQuotes(const QString& text)
    {
        // Match groups on 3 possible conditions:
        //   - Groups with two leading quotes must close both on them (filename:""abc xyz" tuv")
        //   - Groups enclosed in quotes
        //   - Words separated by spaces
        const QRegularExpression subTermsRegExp("(\\S*?\"\"[^\"]+\"[^\"]+\"+|\\S*?\"[^\"]+\"+|(?<=\\s|^)\\S+(?=\\s|$))");
        auto subTermsMatchIterator = subTermsRegExp.globalMatch(text);

        QStringList textParts;
        while (subTermsMatchIterator.hasNext()) {
            textParts << subTermsMatchIterator.next().captured(0);
        }
        return textParts;
    }
#endif
}


DolphinQuery DolphinQuery::fromSearchUrl(const QUrl& searchUrl)
{
    DolphinQuery model;
    model.m_searchUrl = searchUrl;

    if (searchUrl.scheme() == QLatin1String("baloosearch")) {
        model.parseBalooQuery();
    }

    return model;
}

bool DolphinQuery::supportsScheme(const QString& urlScheme)
{
    static const QStringList supportedSchemes = {
        QStringLiteral("baloosearch"),
    };

    return supportedSchemes.contains(urlScheme);
}

void DolphinQuery::parseBalooQuery()
{
#ifdef HAVE_BALOO
    const Baloo::Query query = Baloo::Query::fromSearchUrl(m_searchUrl);

    m_includeFolder = query.includeFolder();

    const QStringList types = query.types();
    m_fileType = types.isEmpty() ? QString() : types.first();

    QStringList textParts;
    QString fileName;

    const QStringList subTerms = splitOutsideQuotes(query.searchString());
    foreach (const QString& subTerm, subTerms) {
        const QString token = searchTermToken(subTerm);
        const QString value = stripQuotes(subTerm.mid(token.length()));

        if (token == QLatin1String("filename:")) {
            if (!value.isEmpty()) {
                fileName = value;
                m_hasFileName = true;
            }
            continue;
        } else if (!token.isEmpty()) {
            m_searchTerms << token + value;
            continue;
        } else if (subTerm == QLatin1String("AND") && subTerm != subTerms.at(0) && subTerm != subTerms.back()) {
            continue;
        } else if (!value.isEmpty()) {
            textParts << value;
            m_hasContentSearch = true;
        }
    }

    if (m_hasFileName) {
        if (m_hasContentSearch) {
            textParts << QStringLiteral("filename:\"%1\"").arg(fileName);
        } else {
            textParts << fileName;
        }
    }

    m_searchText = textParts.join(QLatin1Char(' '));
#endif
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

bool DolphinQuery::hasContentSearch() const
{
    return m_hasContentSearch;
}

bool DolphinQuery::hasFileName() const
{
    return m_hasFileName;
}
