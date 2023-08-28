/*
 * SPDX-FileCopyrightText: 2019 Ismael Asensio <isma.af@mgmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinquery.h"

#include <QRegularExpression>

#include "config-dolphin.h"
#if HAVE_BALOO
#include <Baloo/Query>
#endif

namespace
{
#if HAVE_BALOO
/** Checks if a given term in the Baloo::Query::searchString() is a special search term
 * @return: the specific search token of the term, or an empty QString() if none is found
 */
QString searchTermToken(const QString &term)
{
    static const QLatin1String searchTokens[]{QLatin1String("filename:"),
                                              QLatin1String("modified>="),
                                              QLatin1String("rating>="),
                                              QLatin1String("tag:"),
                                              QLatin1String("tag=")};

    for (const auto &searchToken : searchTokens) {
        if (term.startsWith(searchToken)) {
            return searchToken;
        }
    }
    return QString();
}

QString stripQuotes(const QString &text)
{
    if (text.length() >= 2 && text.at(0) == QLatin1Char('"') && text.back() == QLatin1Char('"')) {
        return text.mid(1, text.size() - 2);
    }
    return text;
}

QStringList splitOutsideQuotes(const QString &text)
{
    // Match groups on 3 possible conditions:
    //   - Groups with two leading quotes must close both on them (filename:""abc xyz" tuv")
    //   - Groups enclosed in quotes
    //   - Words separated by spaces
    static const QRegularExpression subTermsRegExp("(\\S*?\"\"[^\"]+\"[^\"]+\"+|\\S*?\"[^\"]+\"+|(?<=\\s|^)\\S+(?=\\s|$))");
    auto subTermsMatchIterator = subTermsRegExp.globalMatch(text);

    QStringList textParts;
    while (subTermsMatchIterator.hasNext()) {
        textParts << subTermsMatchIterator.next().captured(0);
    }
    return textParts;
}
#endif

QString trimChar(const QString &text, const QLatin1Char aChar)
{
    const int start = text.startsWith(aChar) ? 1 : 0;
    const int end = (text.length() > 1 && text.endsWith(aChar)) ? 1 : 0;

    return text.mid(start, text.length() - start - end);
}
}

DolphinQuery DolphinQuery::fromSearchUrl(const QUrl &searchUrl)
{
    DolphinQuery model;
    model.m_searchUrl = searchUrl;

    if (searchUrl.scheme() == QLatin1String("baloosearch")) {
        model.parseBalooQuery();
    } else if (searchUrl.scheme() == QLatin1String("tags")) {
        // tags can contain # symbols or slashes within the Url
        QString tag = trimChar(searchUrl.toString(QUrl::RemoveScheme), QLatin1Char('/'));
        model.m_searchTerms << QStringLiteral("tag:%1").arg(tag);
    }

    return model;
}

bool DolphinQuery::supportsScheme(const QString &urlScheme)
{
    static const QStringList supportedSchemes = {
        QStringLiteral("baloosearch"),
        QStringLiteral("tags"),
    };

    return supportedSchemes.contains(urlScheme);
}

void DolphinQuery::parseBalooQuery()
{
#if HAVE_BALOO
    const Baloo::Query query = Baloo::Query::fromSearchUrl(m_searchUrl);

    m_includeFolder = query.includeFolder();

    const QStringList types = query.types();
    m_fileType = types.isEmpty() ? QString() : types.first();

    QStringList textParts;
    QString fileName;

    const QStringList subTerms = splitOutsideQuotes(query.searchString());
    for (const QString &subTerm : subTerms) {
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
