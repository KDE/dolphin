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

    QString stripQuotes(const QString& text)
    {
        QString cleanedText = text;
        if (!cleanedText.isEmpty() && cleanedText.at(0) == QLatin1Char('"')) {
            cleanedText = cleanedText.mid(1);
        }
        if (!cleanedText.isEmpty() && cleanedText.back() == QLatin1Char('"')) {
            cleanedText = cleanedText.mid(0, cleanedText.size() - 1);
        }
        return cleanedText;
    }

    QStringList splitOutsideQuotes(const QString& text)
    {
        const QRegularExpression subTermsRegExp("([^ ]*\"[^\"]*\"|(?<= |^)[^ ]+(?= |$))");
        auto subTermsMatchIterator = subTermsRegExp.globalMatch(text);

        QStringList textParts;
        while (subTermsMatchIterator.hasNext()) {
            textParts << subTermsMatchIterator.next().captured(0);
        }
        return textParts;
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
    QString fileName;

    const QStringList subTerms = splitOutsideQuotes(query.searchString());
    foreach (const QString& subTerm, subTerms) {
        if (subTerm.startsWith(QLatin1String("filename:"))) {
            fileName = stripQuotes(subTerm.mid(9));
            if (!fileName.isEmpty()) {
                model.m_hasFileName = true;
            }
            continue;
        } else if (isSearchTerm(subTerm)) {
            model.m_searchTerms << subTerm;
            continue;
        } else if (subTerm == QLatin1String("AND") && subTerm != subTerms.at(0) && subTerm != subTerms.back()) {
            continue;
        } else {
            const QString cleanedTerm = stripQuotes(subTerm);
            if (!cleanedTerm.isEmpty()) {
                textParts << cleanedTerm;
                model.m_hasContentSearch = true;
            }
        }
    }

    if (model.m_hasFileName) {
        if (model.m_hasContentSearch) {
            textParts << QStringLiteral("filename:\"%1\"").arg(fileName);
        } else {
            textParts << fileName;
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

bool DolphinQuery::hasContentSearch() const
{
    return m_hasContentSearch;
}

bool DolphinQuery::hasFileName() const
{
    return m_hasFileName;
}
