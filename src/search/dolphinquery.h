/*
 * SPDX-FileCopyrightText: 2019 Ismael Asensio <isma.af@mgmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
    /** Parses the components of @p searchUrl for the supported schemes */
    static DolphinQuery fromSearchUrl(const QUrl& searchUrl);
    /** Checks whether the DolphinQuery supports the given @p urlScheme */
    static bool supportsScheme(const QString& urlScheme);

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
    /** Calls Baloo::Query::fromSearchUrl() on the current searchUrl
     * and parses the result to extract its separate components */
    void parseBalooQuery();

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
