/*
    SPDX-FileCopyrightText: 2019 Ismael Asensio <isma.af@mgmail.com>
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "search/dolphinquery.h"

#include <QTest>

#include <QDate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

class DolphinQueryTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testBalooSearchParsing_data();
    void testBalooSearchParsing();
    void testExportImport();
};

/**
 * Helper function to compose the baloo query URL used for searching
 */
QUrl balooQueryUrl(const QString &searchString)
{
    const QJsonObject jsonObject{{"searchString", searchString}};

    const QJsonDocument doc(jsonObject);
    const QString queryString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("json"), queryString);

    QUrl searchUrl;
    searchUrl.setScheme(QLatin1String("baloosearch"));
    searchUrl.setQuery(urlQuery);

    return searchUrl;
}

void DolphinQueryTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    Search::setTestMode();
}

/**
 * Defines the parameters for the test cases in testBalooSearchParsing()
 */
void DolphinQueryTest::testBalooSearchParsing_data()
{
    QTest::addColumn<QUrl>("searchUrl");
    QTest::addColumn<QString>("expectedSearchTerm");
    QTest::addColumn<QDate>("expectedModifiedSinceDate");
    QTest::addColumn<int>("expectedMinimumRating");
    QTest::addColumn<QStringList>("expectedTags");
    QTest::addColumn<bool>("hasContent");
    QTest::addColumn<bool>("hasFileName");

    const QString text = QStringLiteral("abc");
    const QString textS = QStringLiteral("abc xyz");
    const QString textQ = QStringLiteral("\"abc xyz\"");
    const QString textM = QStringLiteral("\"abc xyz\" tuv");

    const QString filename = QStringLiteral("filename:\"%1\"").arg(text);
    const QString filenameS = QStringLiteral("filename:\"%1\"").arg(textS);
    const QString filenameQ = QStringLiteral("filename:\"%1\"").arg(textQ);
    const QString filenameM = QStringLiteral("filename:\"%1\"").arg(textM);

    const QString rating = QStringLiteral("rating>=2");
    const QString modified = QStringLiteral("modified>=2019-08-07");
    QDate modifiedDate;
    modifiedDate.setDate(2019, 8, 7);

    const QString tag = QStringLiteral("tag:tagA");
    const QString tagS = QStringLiteral("tag:\"tagB with spaces\""); // in search url
    const QString tagR = QStringLiteral("tag:tagB with spaces"); // in result term

    const QLatin1String tagA{"tagA"};
    const QLatin1String tagBWithSpaces{"tagB with spaces"};

    // Test for "Content"
    QTest::newRow("content") << balooQueryUrl(text) << text << QDate{} << 0 << QStringList() << true << true;
    QTest::newRow("content/space") << balooQueryUrl(textS) << textS << QDate{} << 0 << QStringList() << true << true;
    QTest::newRow("content/quoted") << balooQueryUrl(textQ) << textS << QDate{} << 0 << QStringList() << true << true;
    QTest::newRow("content/empty") << balooQueryUrl("") << "" << QDate{} << 0 << QStringList() << false << false;
    QTest::newRow("content/single_quote") << balooQueryUrl("\"") << "\"" << QDate{} << 0 << QStringList() << true << true;
    QTest::newRow("content/double_quote") << balooQueryUrl("\"\"") << "" << QDate{} << 0 << QStringList() << false << false;

    // Test for "FileName"
    QTest::newRow("filename") << balooQueryUrl(filename) << text << QDate{} << 0 << QStringList() << false << true;
    QTest::newRow("filename/space") << balooQueryUrl(filenameS) << textS << QDate{} << 0 << QStringList() << false << true;
    QTest::newRow("filename/quoted") << balooQueryUrl(filenameQ) << textQ << QDate{} << 0 << QStringList() << false << true;
    QTest::newRow("filename/mixed") << balooQueryUrl(filenameM) << textM << QDate{} << 0 << QStringList() << false << true;
    QTest::newRow("filename/empty") << balooQueryUrl("filename:") << "" << QDate{} << 0 << QStringList() << false << false;
    QTest::newRow("filename/single_quote") << balooQueryUrl("filename:\"") << "\"" << QDate{} << 0 << QStringList() << false << true;
    QTest::newRow("filename/double_quote") << balooQueryUrl("filename:\"\"") << "" << QDate{} << 0 << QStringList() << false << false;

    // Combined content and filename search
    QTest::newRow("content+filename") << balooQueryUrl(text + " " + filename) << text << QDate{} << 0 << QStringList() << true << true;

    QTest::newRow("content+filename/quoted") << balooQueryUrl(textQ + " " + filenameQ) << textS << QDate{} << 0 << QStringList() << true << true;

    // Test for rating
    QTest::newRow("rating") << balooQueryUrl(rating) << "" << QDate{} << 2 << QStringList() << false << false;
    QTest::newRow("rating+content") << balooQueryUrl(rating + " " + text) << text << QDate{} << 2 << QStringList() << true << true;
    QTest::newRow("rating+filename") << balooQueryUrl(rating + " " + filename) << text << QDate{} << 2 << QStringList() << false << true;

    // Test for modified date
    QTest::newRow("modified") << balooQueryUrl(modified) << "" << modifiedDate << 0 << QStringList() << false << false;
    QTest::newRow("modified+content") << balooQueryUrl(modified + " " + text) << text << modifiedDate << 0 << QStringList() << true << true;
    QTest::newRow("modified+filename") << balooQueryUrl(modified + " " + filename) << text << modifiedDate << 0 << QStringList() << false << true;

    // Test for tags
    QTest::newRow("tag") << balooQueryUrl(tag) << "" << QDate{} << 0 << QStringList{tagA} << false << false;
    QTest::newRow("tag/space") << balooQueryUrl(tagS) << "" << QDate{} << 0 << QStringList{tagBWithSpaces} << false << false;
    QTest::newRow("tag/double") << balooQueryUrl(tag + " " + tagS) << "" << QDate{} << 0 << QStringList{tagA, tagBWithSpaces} << false << false;
    QTest::newRow("tag+content") << balooQueryUrl(tag + " " + text) << text << QDate{} << 0 << QStringList{tagA} << true << true;
    QTest::newRow("tag+filename") << balooQueryUrl(tag + " " + filename) << text << QDate{} << 0 << QStringList{tagA} << false << true;

    // Combined search terms
    QTest::newRow("searchTerms") << balooQueryUrl(rating + " AND " + modified + " AND " + tag + " AND " + tagS) << "" << modifiedDate << 2
                                 << QStringList{tagA, tagBWithSpaces} << false << false;

    QTest::newRow("searchTerms+content") << balooQueryUrl(rating + " AND " + modified + " " + text + " " + tag + " AND " + tagS) << text << modifiedDate << 2
                                         << QStringList{tagA, tagBWithSpaces} << true << true;

    QTest::newRow("searchTerms+filename") << balooQueryUrl(rating + " AND " + modified + " " + filename + " " + tag + " AND " + tagS) << text << modifiedDate
                                          << 2 << QStringList{tagA, tagBWithSpaces} << false << true;

    QTest::newRow("allTerms") << balooQueryUrl(text + " " + filename + " " + rating + " AND " + modified + " AND " + tag) << text << modifiedDate << 2
                              << QStringList{tagA} << true << true;

    QTest::newRow("allTerms/space") << balooQueryUrl(textS + " " + filenameS + " " + rating + " AND " + modified + " AND " + tagS) << textS << modifiedDate << 2
                                    << QStringList{tagBWithSpaces} << true << true;

    // Test tags:/ URL scheme
    const auto tagUrl = [](const QString &tag) {
        return QUrl(QStringLiteral("tags:/%1/").arg(tag));
    };

    QTest::newRow("tagsUrl") << tagUrl(tagA) << "" << QDate{} << 0 << QStringList{tagA} << false << false;
    QTest::newRow("tagsUrl/space") << tagUrl(tagBWithSpaces) << "" << QDate{} << 0 << QStringList{tagBWithSpaces} << false << false;
    QTest::newRow("tagsUrl/hash") << tagUrl("tagC#hash") << "" << QDate{} << 0 << QStringList{QStringLiteral("tagC#hash")} << false << false;
    QTest::newRow("tagsUrl/slash") << tagUrl("tagD/with/slash") << "" << QDate{} << 0 << QStringList{QStringLiteral("tagD/with/slash")} << false << false;
}

/**
 * The test verifies whether the different terms search URL (e.g. "baloosearch:/") are
 * properly handled by the searchbox, and only "user" or filename terms are added to the
 * text bar of the searchbox.
 */
void DolphinQueryTest::testBalooSearchParsing()
{
    QFETCH(QUrl, searchUrl);
    QFETCH(QString, expectedSearchTerm);
    QFETCH(QDate, expectedModifiedSinceDate);
    QFETCH(int, expectedMinimumRating);
    QFETCH(QStringList, expectedTags);
    QFETCH(bool, hasContent);
    QFETCH(bool, hasFileName);

    const Search::DolphinQuery query = Search::DolphinQuery{searchUrl, /** No backupSearchPath should be needed because searchUrl should be valid. */ QUrl{}};

    // Checkt that the URL is supported
    QVERIFY(Search::isSupportedSearchScheme(searchUrl.scheme()));

    // Check for parsed text (would be displayed on the input search bar)
    QCOMPARE(query.searchTerm(), expectedSearchTerm);

    QCOMPARE(query.modifiedSinceDate(), expectedModifiedSinceDate);

    QCOMPARE(query.minimumRating(), expectedMinimumRating);

    QCOMPARE(query.requiredTags(), expectedTags);

    // Check that there were no unrecognized baloo query parameters in the above strings.
    Q_ASSERT(query.m_unrecognizedBalooQueryStrings.isEmpty());

    // Check if a search term is looked up in the file names or contents
    QCOMPARE(query.searchThrough() == Search::SearchThrough::FileContents && !query.searchTerm().isEmpty(), hasContent);
    QCOMPARE(!query.searchTerm().isEmpty(), hasFileName); // The file names are always also searched even when searching through file contents.
}

/**
 * Tests whether exporting a DolphinQuery object to a URL and then constructing a DolphinQuery object from that URL recreates the same DolphinQuery.
 */
void DolphinQueryTest::testExportImport()
{
    /// Initialize the DolphinQuery with some standard settings.
    const QUrl searchPath1{"file:///someNonExistentUrl"};
    Search::DolphinQuery query{searchPath1, searchPath1};
    query.setSearchLocations(Search::SearchLocations::FromHere);
    QVERIFY(query.searchLocations() == Search::SearchLocations::FromHere);
    query.setSearchThrough(Search::SearchThrough::FileNames);
    QVERIFY(query.searchThrough() == Search::SearchThrough::FileNames);
    query.setSearchTool(Search::SearchTool::Filenamesearch);
    QVERIFY(query.searchTool() == Search::SearchTool::Filenamesearch);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), searchPath1)); // Export then import

    /// Test that exporting and importing works as expected no matter which aspect we change.
    query.setSearchThrough(Search::SearchThrough::FileContents);
    QVERIFY(query.searchThrough() == Search::SearchThrough::FileContents);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), searchPath1)); // Export then import

    constexpr QLatin1String searchTerm1{"abc"};
    query.setSearchTerm(searchTerm1);
    QVERIFY(query.searchTerm() == searchTerm1);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), searchPath1)); // Export then import

    query.setSearchThrough(Search::SearchThrough::FileNames);
    QVERIFY(query.searchThrough() == Search::SearchThrough::FileNames);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), searchPath1)); // Export then import

    QVERIFY(query.searchPath() == searchPath1);
    const QUrl searchPath2{"file:///someNonExistentUrl2"};
    query.setSearchPath(searchPath2);
    QVERIFY(query.searchPath() == searchPath2);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because otherUrl is imported.

    query.setSearchLocations(Search::SearchLocations::Everywhere);
    QVERIFY(query.searchLocations() == Search::SearchLocations::Everywhere);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), searchPath2)); // Export then import. searchPath2 is required to match as the fallback.

    QVERIFY(query.searchTerm() == searchTerm1);
    constexpr QLatin1String searchTerm2{"xyz"};
    query.setSearchTerm(searchTerm2);
    QVERIFY(query.searchTerm() == searchTerm2);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), searchPath2)); // Export then import

    QVERIFY(query.searchLocations() == Search::SearchLocations::Everywhere);
    query.setSearchLocations(Search::SearchLocations::FromHere);
    QVERIFY(query.searchLocations() == Search::SearchLocations::FromHere);
    QVERIFY(query.searchPath() == searchPath2);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath2 is imported.

#if HAVE_BALOO
    /// Test Baloo search queries
    query.setSearchTool(Search::SearchTool::Baloo);
    QVERIFY(query.searchTool() == Search::SearchTool::Baloo);
    QVERIFY(query.searchTerm() == searchTerm2);
    QVERIFY(query.searchLocations() == Search::SearchLocations::FromHere);
    QVERIFY(query.searchPath() == searchPath2);
    QVERIFY(query.searchThrough() == Search::SearchThrough::FileNames);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath2 is imported.

    /// Test that exporting and importing works as expected no matter which aspect we change.
    query.setSearchThrough(Search::SearchThrough::FileContents);
    QVERIFY(query.searchThrough() == Search::SearchThrough::FileContents);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath2 is imported.

    query.setSearchTerm(searchTerm1);
    QVERIFY(query.searchTerm() == searchTerm1);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath2 is imported.

    query.setSearchThrough(Search::SearchThrough::FileNames);
    QVERIFY(query.searchThrough() == Search::SearchThrough::FileNames);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath2 is imported.

    QVERIFY(query.searchPath() == searchPath2);
    query.setSearchPath(searchPath1);
    QVERIFY(query.searchPath() == searchPath1);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath2 is imported.

    query.setSearchLocations(Search::SearchLocations::Everywhere);
    QVERIFY(query.searchLocations() == Search::SearchLocations::Everywhere);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), searchPath1)); // Export then import. searchPath1 is required to match as the fallback.

    QVERIFY(query.searchTerm() == searchTerm1);
    query.setSearchTerm(searchTerm2);
    QVERIFY(query.searchTerm() == searchTerm2);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), searchPath1)); // Export then import

    QVERIFY(query.searchLocations() == Search::SearchLocations::Everywhere);
    query.setSearchLocations(Search::SearchLocations::FromHere);
    QVERIFY(query.searchLocations() == Search::SearchLocations::FromHere);
    QVERIFY(query.searchPath() == searchPath1);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath1 is imported.

    QVERIFY(query.fileType() == KFileMetaData::Type::Empty);
    query.setFileType(KFileMetaData::Type::Archive);
    QVERIFY(query.fileType() == KFileMetaData::Type::Archive);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath1 is imported.

    QVERIFY(!query.modifiedSinceDate().isValid());
    QDate modifiedDate;
    modifiedDate.setDate(2018, 6, 3); // World Bicycle Day
    query.setModifiedSinceDate(modifiedDate);
    QVERIFY(query.modifiedSinceDate() == modifiedDate);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath1 is imported.

    QVERIFY(query.minimumRating() == 0);
    query.setMinimumRating(4);
    QVERIFY(query.minimumRating() == 4);
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath1 is imported.

    QVERIFY(query.requiredTags().isEmpty());
    query.setRequiredTags({searchTerm1, searchTerm2});
    QVERIFY(query.requiredTags().contains(searchTerm1));
    QVERIFY(query.requiredTags().contains(searchTerm2));
    QVERIFY(query == Search::DolphinQuery(query.toUrl(), QUrl{})); // Export then import. The QUrl{} fallback does not matter because searchPath1 is imported.

    QVERIFY(query.searchTool() == Search::SearchTool::Baloo);
    QVERIFY(query.searchThrough() == Search::SearchThrough::FileNames);
    QVERIFY(query.searchPath() == searchPath1);
    QVERIFY(query.searchTerm() == searchTerm2);
    QVERIFY(query.searchLocations() == Search::SearchLocations::FromHere);
    QVERIFY(query.fileType() == KFileMetaData::Type::Archive);
    QVERIFY(query.modifiedSinceDate() == modifiedDate);
    QVERIFY(query.minimumRating() == 4);

    /// Changing the search tool should not immediately drop all the extra information even if the search tool might not support searching for them.
    /// This is mostly an attempt to not drop properties set by the user earlier than we have to.
    query.setSearchTool(Search::SearchTool::Filenamesearch);
    QVERIFY(query.searchThrough() == Search::SearchThrough::FileNames);
    QVERIFY(query.searchPath() == searchPath1);
    QVERIFY(query.searchTerm() == searchTerm2);
    QVERIFY(query.searchLocations() == Search::SearchLocations::FromHere);
    QVERIFY(query.fileType() == KFileMetaData::Type::Archive);
    QVERIFY(query.modifiedSinceDate() == modifiedDate);
    QVERIFY(query.minimumRating() == 4);
#endif
}

QTEST_MAIN(DolphinQueryTest)

#include "dolphinquerytest.moc"
