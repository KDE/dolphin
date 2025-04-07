/*
    SPDX-FileCopyrightText: 2019 Ismael Asensio <isma.af@mgmail.com>
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dolphinquery.h"

#include "config-dolphin.h"
#if HAVE_BALOO
#include <Baloo/IndexerConfig>
#include <Baloo/Query>
#endif
#include "dolphinplacesmodelsingleton.h"

#include <KFileMetaData/TypeInfo>
#include <KLocalizedString>

#include <QRegularExpression>
#include <QUrlQuery>

using namespace Search;

bool Search::isSupportedSearchScheme(const QString &urlScheme)
{
    static const QStringList supportedSchemes = {
        QStringLiteral("filenamesearch"),
        QStringLiteral("baloosearch"),
        QStringLiteral("tags"),
    };

    return supportedSchemes.contains(urlScheme);
}

bool g_testMode = false;

bool Search::isIndexingEnabledIn(QUrl directory)
{
    if (g_testMode) {
        return true; // For unit-testing, let's pretend everything is indexed correctly.
    }

#if HAVE_BALOO
    const Baloo::IndexerConfig searchInfo;
    return searchInfo.fileIndexingEnabled() && !directory.isEmpty() && searchInfo.shouldBeIndexed(directory.toLocalFile());
#else
    Q_UNUSED(directory)
    return false;
#endif
}

bool Search::isContentIndexingEnabled()
{
    if (g_testMode) {
        return true; // For unit-testing, let's pretend everything is indexed correctly.
    }

#if HAVE_BALOO
    return !Baloo::IndexerConfig{}.onlyBasicIndexing();
#else
    return false;
#endif
}

namespace
{
/** The path to be passed so Baloo searches everywhere. */
constexpr auto balooSearchEverywherePath = QLatin1String("");
/** The path to be passed so Filenamesearch searches everywhere. */
constexpr auto filenamesearchEverywherePath = QLatin1String("file:///");

#if HAVE_BALOO
/** Checks if a given term in the Baloo::Query::searchString() is a special search term
 * @return: the specific search token of the term, or an empty QString() if none is found
 */
QString searchTermToken(const QString &term)
{
    static const QLatin1String searchTokens[]{QLatin1String("filename:"),
                                              QLatin1String("modified>="),
                                              QLatin1String("modified>"),
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

Search::DolphinQuery::DolphinQuery(const QUrl &url, const QUrl &backupSearchPath)
{
    if (url.scheme() == QLatin1String("filenamesearch")) {
        m_searchTool = SearchTool::Filenamesearch;
        const QUrlQuery query(url);
        const QString filenamesearchSearchPathString = query.queryItemValue(QStringLiteral("url"), QUrl::FullyDecoded);
        const QUrl filenamesearchSearchPathUrl = QUrl::fromUserInput(filenamesearchSearchPathString, QString(), QUrl::AssumeLocalFile);
        if (!filenamesearchSearchPathUrl.isValid() || filenamesearchSearchPathString == filenamesearchEverywherePath) {
            // The parsed search location is either invalid or matches a string that represents searching "everywhere".
            m_searchLocations = SearchLocations::Everywhere;
            m_searchPath = backupSearchPath;
        } else {
            m_searchLocations = SearchLocations::FromHere;
            m_searchPath = filenamesearchSearchPathUrl;
        }
        m_searchTerm = query.queryItemValue(QStringLiteral("search"), QUrl::FullyDecoded);
        m_searchThrough = query.queryItemValue(QStringLiteral("checkContent")) == QLatin1String("yes") ? SearchThrough::FileContents : SearchThrough::FileNames;
        return;
    }

#if HAVE_BALOO
    if (url.scheme() == QLatin1String("baloosearch")) {
        m_searchTool = SearchTool::Baloo;
        initializeFromBalooQuery(Baloo::Query::fromSearchUrl(url), backupSearchPath);
        return;
    }
#endif

    if (url.scheme() == QLatin1String("tags")) {
#if HAVE_BALOO
        m_searchTool = SearchTool::Baloo;
#endif
        m_searchLocations = SearchLocations::Everywhere;
        m_searchPath = backupSearchPath;
        // tags can contain # symbols or slashes within the Url
        const auto tag = trimChar(url.toString(QUrl::RemoveScheme), QLatin1Char('/'));
        if (!tag.isEmpty()) {
            m_requiredTags.append(trimChar(url.toString(QUrl::RemoveScheme), QLatin1Char('/')));
        }
        return;
    }

    m_searchPath = url;
    switchToPreferredSearchTool();
}

QUrl DolphinQuery::toUrl() const
{
    // The following pre-conditions are sanity checks on this DolphinQuery object. If they fail, the issue is that we ever allowed the DolphinQuery to be in an
    // inconsistent state to begin with. This should be fixed by bringing this DolphinQuery object into a reasonable state at the end of the constructors or
    // setter methods which caused this impossible-to-fulfill combination of conditions.
    Q_ASSERT_X(m_searchLocations == SearchLocations::Everywhere || m_searchPath.isValid(),
               "DolphinQuery::toUrl()",
               "We are supposed to search in a specific location but we do not know where!");
#if HAVE_BALOO
    Q_ASSERT_X(m_searchLocations == SearchLocations::Everywhere || m_searchTool != SearchTool::Baloo || isIndexingEnabledIn(m_searchPath),
               "DolphinQuery::toUrl()",
               "We are asking Baloo to search in a location which Baloo is not supposed to have indexed!");
#endif

    QUrl url;

#if HAVE_BALOO
    /// Create Baloo search URL
    if (m_searchTool == SearchTool::Baloo) {
        Baloo::Query query;
        if (m_fileType != KFileMetaData::Type::Empty) {
            query.addType(KFileMetaData::TypeInfo{m_fileType}.name());
        }

        QStringList balooQueryStrings = m_unrecognizedBalooQueryStrings;

        if (m_searchThrough == SearchThrough::FileContents) {
            balooQueryStrings << m_searchTerm;
        } else if (!m_searchTerm.isEmpty()) {
            balooQueryStrings << QStringLiteral("filename:\"%1\"").arg(m_searchTerm);
        }

        if (m_searchLocations == SearchLocations::FromHere) {
            query.setIncludeFolder(m_searchPath.toLocalFile());
        }

        if (m_modifiedSinceDate.isValid()) {
            balooQueryStrings << QStringLiteral("modified>=%1").arg(m_modifiedSinceDate.toString(Qt::ISODate));
        }

        if (m_minimumRating >= 1) {
            balooQueryStrings << QStringLiteral("rating>=%1").arg(m_minimumRating);
        }

        for (const auto &tag : m_requiredTags) {
            if (tag.contains(QLatin1Char(' '))) {
                balooQueryStrings << QStringLiteral("tag:\"%1\"").arg(tag);
            } else {
                balooQueryStrings << QStringLiteral("tag:%1").arg(tag);
            }
        }

        query.setSearchString(balooQueryStrings.join(QLatin1Char(' ')));

        return query.toSearchUrl(QUrl::toPercentEncoding(title()));
    }
#endif

    /// Create Filenamsearch search URL
    url.setScheme(QStringLiteral("filenamesearch"));

    QUrlQuery qUrlQuery;
    qUrlQuery.addQueryItem(QStringLiteral("search"), QUrl::toPercentEncoding(m_searchTerm));
    if (m_searchThrough == SearchThrough::FileContents) {
        qUrlQuery.addQueryItem(QStringLiteral("checkContent"), QStringLiteral("yes"));
    }

    if (m_searchLocations == SearchLocations::FromHere && m_searchPath.isValid()) {
        qUrlQuery.addQueryItem(QStringLiteral("url"), QUrl::toPercentEncoding(m_searchPath.url()));
    } else {
        // Search in root which is considered searching "everywhere".
        qUrlQuery.addQueryItem(QStringLiteral("url"), QUrl::toPercentEncoding(filenamesearchEverywherePath));
    }
    qUrlQuery.addQueryItem(QStringLiteral("title"), QUrl::toPercentEncoding(title()));

    url.setQuery(qUrlQuery);
    return url;
}

void DolphinQuery::setSearchLocations(SearchLocations searchLocations)
{
    m_searchLocations = searchLocations;
    switchToPreferredSearchTool();
}

void DolphinQuery::setSearchPath(const QUrl &searchPath)
{
    m_searchPath = searchPath;
    switchToPreferredSearchTool();
}

void DolphinQuery::setSearchThrough(SearchThrough searchThrough)
{
    m_searchThrough = searchThrough;
    switchToPreferredSearchTool();
}

void DolphinQuery::switchToPreferredSearchTool()
{
    const bool isIndexingEnabledInCurrentSearchLocation = m_searchLocations == SearchLocations::Everywhere || isIndexingEnabledIn(m_searchPath);
    const bool searchThroughFileContentsWithoutIndexing = m_searchThrough == SearchThrough::FileContents && !isContentIndexingEnabled();
    if (!isIndexingEnabledInCurrentSearchLocation || searchThroughFileContentsWithoutIndexing) {
        m_searchTool = SearchTool::Filenamesearch;
        return;
    }
#if HAVE_BALOO
    // The current search location allows searching with Baloo. We switch to Baloo if this is the saved user preference.
    if (SearchSettings::searchTool() == QStringLiteral("Baloo")) {
        m_searchTool = SearchTool::Baloo;
    }
#endif
}

#if HAVE_BALOO
void DolphinQuery::initializeFromBalooQuery(const Baloo::Query &balooQuery, const QUrl &backupSearchPath)
{
    const QString balooSearchPathString = balooQuery.includeFolder();
    const QUrl balooSearchPathUrl = QUrl::fromUserInput(balooSearchPathString, QString(), QUrl::AssumeLocalFile);
    if (!balooSearchPathUrl.isValid() || balooSearchPathString == balooSearchEverywherePath) {
        // The parsed search location is either invalid or matches a string that represents searching "everywhere" i.e. in all indexed locations.
        m_searchLocations = SearchLocations::Everywhere;
        m_searchPath = backupSearchPath;
    } else {
        m_searchLocations = SearchLocations::FromHere;
        m_searchPath = balooSearchPathUrl;
    }

    const QStringList types = balooQuery.types();
    // We currently only allow searching for one file type at once. (Searching for more seems out of scope for Dolphin anyway IMO.)
    m_fileType = types.isEmpty() ? KFileMetaData::Type::Empty : KFileMetaData::TypeInfo::fromName(types.first()).type();

    /// If nothing is requested, we use the default.
    std::optional<SearchThrough> requestedToSearchThrough;
    const QStringList subTerms = splitOutsideQuotes(balooQuery.searchString());
    for (const QString &subTerm : subTerms) {
        const QString token = searchTermToken(subTerm);
        const QString value = stripQuotes(subTerm.mid(token.length()));

        if (token == QLatin1String("filename:")) {
            // This query is meant to not search in file contents.
            if (!value.isEmpty()) {
                if (m_searchTerm.isEmpty()) { // Seems like we already received a search term for the content search. We don't overwrite it because the Dolphin
                                              // UI does not support searching for differing strings in content and file name.
                    m_searchTerm = value;
                }
                if (!requestedToSearchThrough.has_value()) { // If requested to search thorugh contents, searching file names is already implied.
                    requestedToSearchThrough = SearchThrough::FileNames;
                }
            }
            continue;
        } else if (token.startsWith(QLatin1String("modified>="))) {
            m_modifiedSinceDate = QDate::fromString(value, Qt::ISODate);
            continue;
        } else if (token.startsWith(QLatin1String("modified>"))) {
            m_modifiedSinceDate = QDate::fromString(value, Qt::ISODate).addDays(1);
            continue;
        } else if (token.startsWith(QLatin1String("rating>="))) {
            m_minimumRating = value.toInt();
            continue;
        } else if (token.startsWith(QLatin1String("tag"))) {
            m_requiredTags.append(value);
            continue;
        } else if (!token.isEmpty()) {
            m_unrecognizedBalooQueryStrings << token + value;
            continue;
        } else if (subTerm == QLatin1String("AND") && subTerm != subTerms.at(0) && subTerm != subTerms.back()) {
            continue;
        } else if (!value.isEmpty()) {
            // An empty token means this is just blank text, which is where the generic search term is located.
            if (!m_searchTerm.isEmpty()) {
                // Multiple search terms are separated by spaces.
                m_searchTerm.append(QLatin1Char{' '});
            }
            m_searchTerm.append(value);
            requestedToSearchThrough = SearchThrough::FileContents;
        }
    }
    if (requestedToSearchThrough.has_value()) {
        m_searchThrough = requestedToSearchThrough.value();
    }
}
#endif // HAVE_BALOO

QString DolphinQuery::title() const
{
    if (m_searchLocations == SearchLocations::FromHere) {
        QString prettySearchLocation;
        KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();
        QModelIndex url_index = placesModel->closestItem(m_searchPath);
        if (url_index.isValid() && placesModel->url(url_index).matches(m_searchPath, QUrl::StripTrailingSlash)) {
            prettySearchLocation = placesModel->text(url_index);
        } else {
            prettySearchLocation = m_searchPath.fileName();
        }
        if (prettySearchLocation.isEmpty()) {
            prettySearchLocation = m_searchPath.toString(QUrl::RemoveAuthority);
        }

        // A great title clearly identifies a search results page among many tabs, windows, or links in the Places panel.
        // Using the search term is phenomenal for this because the user has typed this very specific string with a clear intention, so we definitely want to
        // reuse the search term in the title if possible.
        if (!m_searchTerm.isEmpty()) {
            if (m_searchThrough == SearchThrough::FileNames) {
                return i18nc("@title of a search results page. %1 is the search term a user entered, %2 is a folder name",
                             "Search results for “%1” in %2",
                             m_searchTerm,
                             prettySearchLocation);
            }
            Q_ASSERT(m_searchThrough == SearchThrough::FileContents);
            return i18nc("@title of a search results page. %1 is the search term a user entered, %2 is a folder name",
                         "Files containing “%1” in %2",
                         m_searchTerm,
                         prettySearchLocation);
        }
        if (!m_requiredTags.isEmpty()) {
            if (m_requiredTags.count() == 1) {
                return i18nc("@title of a search results page. %1 is a tag e.g. 'important'. %2 is a folder name",
                             "Search items tagged “%1” in %2",
                             m_requiredTags.constFirst(),
                             prettySearchLocation);
            }
            return i18nc("@title of a search results page. %1 and %2 are tags e.g. 'important'. %3 is a folder name",
                         "Search items tagged “%1” and “%2” in %3",
                         m_requiredTags.constFirst(),
                         m_requiredTags.constLast(),
                         prettySearchLocation);
        }
        if (m_fileType != KFileMetaData::Type::Empty) {
            return i18nc("@title of a search results page for items of a specified type. %1 is a file type e.g. 'Document', 'Folder'. %2 is a folder name",
                         "%1 search results in %2",
                         KFileMetaData::TypeInfo{m_fileType}.displayName(),
                         prettySearchLocation);
        }
        // Everything else failed so we use a very generic title.
        return i18nc("@title of a search results page with items matching pre-defined conditions. %1 is a folder name",
                     "Search results in %1",
                     prettySearchLocation);
    }

    Q_ASSERT(m_searchLocations == SearchLocations::Everywhere);
    // A great title clearly identifies a search results page among many tabs, windows, or links in the Places panel.
    // Using the search term is phenomenal for this because the user has typed this very specific string with a clear intention, so we definitely want to reuse
    // the search term in the title if possible.
    if (!m_searchTerm.isEmpty()) {
        if (m_searchThrough == SearchThrough::FileNames) {
            return i18nc("@title of a search results page. %1 is the search term a user entered", "Search results for “%1”", m_searchTerm);
        }
        Q_ASSERT(m_searchThrough == SearchThrough::FileContents);
        return i18nc("@title of a search results page. %1 is the search term a user entered", "Files containing “%1”", m_searchTerm);
    }
    if (!m_requiredTags.isEmpty()) {
        if (m_requiredTags.count() == 1) {
            return i18nc("@title of a search results page. %1 is a tag e.g. 'important'", "Search items tagged “%1”", m_requiredTags.constFirst());
        }
        return i18nc("@title of a search results page. %1 and %2 are tags e.g. 'important'",
                     "Search items tagged “%1” and “%2”",
                     m_requiredTags.constFirst(),
                     m_requiredTags.constLast());
    }
    if (m_fileType != KFileMetaData::Type::Empty) {
        // i18n: Results page for items of a specified type. %1 is a file type e.g. 'Audio', 'Document', 'Folder', 'Archive'. 'Presentation'.
        // If putting such a file type at the start does not work in your language in this context, you might want to translate this liberally with
        // something along the lines of 'Search items of type “%1”'.
        return i18nc("@title of a search. %1 is file type", "%1 search results", KFileMetaData::TypeInfo{m_fileType}.displayName());
    }
    // Everything else failed so we use a very generic title.
    return i18nc("@title of a search results page with items matching pre-defined conditions", "Search results");
}

/** For testing Baloo in DolphinQueryTest even if it is not indexing any locations yet. */
void Search::setTestMode()
{
    g_testMode = true;
};
