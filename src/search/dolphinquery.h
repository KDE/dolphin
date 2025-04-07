/*
    SPDX-FileCopyrightText: 2019 Ismael Asensio <isma.af@mgmail.com>
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef DOLPHINQUERY_H
#define DOLPHINQUERY_H

#include "config-dolphin.h"
#include "dolphin_export.h"
#include "dolphin_searchsettings.h"

#include <KFileMetaData/Types>

#include <QString>
#include <QUrl>

#if HAVE_BALOO
namespace Baloo
{
class Query;
}
#endif

class DolphinQueryTest;

namespace Search
{

/** Specifies which locations the user expects to be searched for matches. */
enum class SearchLocations {
    FromHere, /// Search in m_searchUrl and its sub-folders.
    Everywhere, /// Search "Everywhere" as far as possible.
};

/** Specifies if items should be added to the search results when their file names or contents matches the search term. */
enum class SearchThrough {
    FileNames,
    FileContents, // This option currently also includes any searches that search both through FileContents and FileNames at once because we currently provide
                  // no option to toggle between only searching FileContents or FileContents & FileNames for any search tool.
};

enum class SearchTool {
    Filenamesearch, // Contrary to its name, it can actually also search in file contents.
#if HAVE_BALOO
    Baloo,
#endif
};

/** @returns whether Baloo is configured to have indexed the @p directory. */
bool isIndexingEnabledIn(QUrl directory);

/** @returns whether Baloo is configured to index file contents. */
bool isContentIndexingEnabled();

/** @returns whether the @p urlScheme should be considered a search scheme. */
bool isSupportedSearchScheme(const QString &urlScheme);

/**
 * @brief An object that fully specifies a search configuration.
 *
 * A DolphinQuery encompasses all state information to uniquely identify a search. It describes the search term, search tool, search options, and requirements
 * towards results. As such it can fully contain all state information of the Search::Bar because the search bars only goal is configuring and then triggering
 * a search.
 *
 * The @a toUrl() method constructs a search URL from the DolphinQuery which Dolphin can open to start searching. Such a search URL can also be transformed
 * back into a DolphinQuery object through the DolphinQuery constructor.
 *
 * When a DolphinQuery object is constructed or changed with incompatible conditions, like asking to search with an index-based search tool in a search path
 * which is not indexed, DolphinQuery tries to fix itself so the final search can give meaningful results.
 * Another example of this would be a DolphinQuery that is restricted to only search for images, which is then changed to use a search tool which does not
 * allow restricting results to images. In that case the DolphinQuery object would not necessarily need to fix itself, but the exported search URL will ignore
 * the image restriction, and the search user interface will need to update itself to make clear that the image restriction is ignored.
 *
 * Most widgets in the search UI have a `updateState()` method that takes a DolphinQuery as an argument. These methods will update the components' state to be
 * in line with the DolphinQuery object's configuration.
 */
class DolphinQuery
{
public:
    /**
     * @brief Automagically constructs a DolphinQuery based on the given @p url.
     * @param url In the most usual case @p url is considered the search path and the DolphinQuery object is initialized based on saved user preferences.
     *            However, if the @p url has query information encoded in itself, which is supposed to be the case if the QUrl::scheme() of the @p url is
     *            "baloosearch", "tags", or "filenamesearch", this constructor retrieves all the information from the @p url and initializes the DolphinQuery
     *            with it.
     * @param backupSearchPath The last non-search location the user was on.
     *                         A DolphinQuery object should always be fully constructible from the main @p url parameter. However, the data encoded in @url
     *                         might not contain any search path, for example because the constructed DolphinQuery object is supposed to search "everywhere".
     *                         This is fine until this DolphinQuery object is switched to search in a specific location instead. In that case, this
     *                         @p backupSearchPath will become the new searchPath() of this DolphinQuery.
     */
    explicit DolphinQuery(const QUrl &url, const QUrl &backupSearchPath);

    /**
     * @returns a representation of this DolphinQuery as a QUrl. This QUrl can be opened in Dolphin to trigger a search that is identical to the conditions
     *          provided by this DolphinQuery object.
     */
    QUrl toUrl() const;

    void setSearchLocations(SearchLocations searchLocations);
    inline SearchLocations searchLocations() const
    {
        return m_searchLocations;
    };

    /**
     * Set this query to search in @p searchPath. However, if @a searchLocations() is set to "Everywhere", @p searchPath is effectively ignored because it is
     * assumed that searching everywhere also includes @p searchPath.
     */
    void setSearchPath(const QUrl &searchPath);
    /**
     * @returns in which specific directory this query will search if the search location is not set to "Everywhere". When searching "Everywhere" this url is
     *          ignored completely.
     */
    inline QUrl searchPath() const
    {
        return m_searchPath;
    };

    /**
     * Set whether search results should match the search term with their names or contain it in their file contents.
     */
    void setSearchThrough(SearchThrough searchThrough);
    inline SearchThrough searchThrough() const
    {
        return m_searchThrough;
    };

    /**
     * Set the search tool or backend that will be used to @p searchTool.
     */
    inline void setSearchTool(SearchTool searchTool)
    {
        m_searchTool = searchTool;
        // We do not remove any search parameters here, even if the new search tool does not support them. This is an attempt to avoid that we unnecessarily
        // throw away configuration data. Non-applicable search parameters will be lost when exporting this DolphinQuery to a URL,
        // but such an export won't happen if the changed DolphinQuery is not a valid search e.g. because the searchTerm().isEmpty() and every other search
        // parameter is not supported by the new search tool.
    };
    /** @returns the search tool to be used for this search. */
    inline SearchTool searchTool() const
    {
        return m_searchTool;
    };

    /**
     * Sets the search text the user entered into the search field to @p searchTerm.
     */
    inline void setSearchTerm(const QString &searchTerm)
    {
        m_searchTerm = searchTerm;
    };
    /** @return the search text the user entered into the search field. */
    inline QString searchTerm() const
    {
        return m_searchTerm;
    };

    /**
     * Sets the type every search result should have.
     */
    inline void setFileType(const KFileMetaData::Type::Type &fileType)
    {
        m_fileType = fileType;
    };
    /**
     * @return the requested file type this search will be restricted to.
     */
    inline KFileMetaData::Type::Type fileType() const
    {
        return m_fileType;
    };

    /**
     * Sets the date since when every search result needs to have been modified.
     */
    inline void setModifiedSinceDate(const QDate &modifiedLaterThanDate)
    {
        m_modifiedSinceDate = modifiedLaterThanDate;
    };
    /**
     * @return the date since when every search result needs to have been modified.
     */
    inline QDate modifiedSinceDate() const
    {
        return m_modifiedSinceDate;
    };

    /**
     * @param minimumRating the minimum rating value every search result needs to at least have to be considered a valid result of this query.
     *                      Values <= 0 mean no restriction. 1 is half a star, 2 one full star, etc. 10 is typically the maximum in KDE software.
     */
    inline void setMinimumRating(int minimumRating)
    {
        m_minimumRating = minimumRating;
    };
    /**
     * @returns the minimum rating every search result is requested to have.
     * @see setMinimumRating().
     */
    inline int minimumRating() const
    {
        return m_minimumRating;
    };

    /**
     * @param requiredTags All the tags every search result is required to have.
     */
    inline void setRequiredTags(const QStringList &requiredTags)
    {
        m_requiredTags = requiredTags;
    };
    /**
     * @returns all the tags every search result is required to have.
     */
    inline QStringList requiredTags() const
    {
        return m_requiredTags;
    };

    bool operator==(const DolphinQuery &) const = default;

    /**
     * @returns a title to be used in user-facing situations to represent this DolphinQuery, such as "Query Results from 'importantFile'".
     */
    QString title() const;

private:
#if HAVE_BALOO
    /** Parses a Baloo::Query to extract its separate components */
    void initializeFromBalooQuery(const Baloo::Query &balooQuery, const QUrl &backupSearchPath);
#endif

    /**
     * Switches to the user's preferred search tool if this is possible. If the preferred search tool cannot perform a search within this DolphinQuery's
     * conditions, a different search tool will be used instead.
     */
    void switchToPreferredSearchTool();

private:
    /** Specifies which locations will be searched for the search terms. */
    SearchLocations m_searchLocations = SearchSettings::location() == QLatin1String("Everywhere") ? SearchLocations::Everywhere : SearchLocations::FromHere;

    /**
     * Specifies where searching will begin.
     * @note The value of this variable is ignored when this query is set to search "Everywhere".
     */
    QUrl m_searchPath;

    /** Specifies whether file names, file contents, or both will be searched for the search terms. */
    SearchThrough m_searchThrough = SearchSettings::what() == QLatin1String("FileContents") ? SearchThrough::FileContents : SearchThrough::FileNames;

    /** Specifies which search tool will be used for the search. */
#if HAVE_BALOO
    SearchTool m_searchTool = SearchSettings::searchTool() == QLatin1String("Baloo") ? SearchTool::Baloo : SearchTool::Filenamesearch;
#else
    SearchTool m_searchTool = SearchTool::Filenamesearch;
#endif

    QString m_searchTerm;
    /** Specifies which file type all search results should have. "Empty" means there is no restriction on file type. */
    KFileMetaData::Type::Type m_fileType = KFileMetaData::Type::Empty;

    /** All search results are requested to be modified later than or equal to this date. Null or invalid dates mean no restriction. */
    QDate m_modifiedSinceDate;

    /**
     * All search results are requested to have at least this rating.
     * If the minimum rating is less than or equal to 0, this variable is ignored.
     * 1 is generally considered half a star in KDE software, 2 a full star, etc. Generally 10 is considered the max rating i.e. 5/5 stars or a song marked as
     * one of your favourites in a music application. Higher values are AFAIK not used in KDE applications but other software might use a different scale.
     */
    int m_minimumRating = 0;

    /** All the tags every search result is required to have. */
    QStringList m_requiredTags;

    /**
     * @brief Any imported Baloo search parameters (token:value pairs) which Dolphin does not understand are stored in this list unmodified.
     * Dolphin only allows modifying a certain selection of search parameters, but there are more. This is a bit of an unfortunate situation because we can not
     * represent every single query in the user interface without creating a mess of a UI. However, we also don't want to drop these extra parameters because
     * that would unexpectedly modify the query.
     * So this variable simply stores anything we don't recognize and reproduces it when exporting to a Baloo URL.
     */
    QStringList m_unrecognizedBalooQueryStrings;

    friend DolphinQueryTest;
};

/**
 * For testing Baloo in DolphinQueryTest even if it is not indexing any locations yet.
 * The test mode makes sure that DolphinQuery can be set to use Baloo even if Baloo has not indexed any locations yet.
 */
void setTestMode();
}

#endif //DOLPHINQUERY_H
