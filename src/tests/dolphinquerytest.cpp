/***************************************************************************
 *   Copyright (C) 2019 by Ismael Asensio <isma.af@gmail.com>              *
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

#include "search/dolphinquery.h"

#include <QTest>

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QUrl>
#include <QUrlQuery>

class DolphinSearchBoxTest : public QObject
{
    Q_OBJECT

private slots:
    void testBalooSearchParsing_data();
    void testBalooSearchParsing();
};

/**
 * Helper function to compose the baloo query URL used for searching
 */
QUrl balooQueryUrl(const QString& searchString)
{
    const QJsonObject jsonObject {
        {"searchString", searchString}
    };

    const QJsonDocument doc(jsonObject);
    const QString queryString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    QUrlQuery urlQuery;
    urlQuery.addQueryItem(QStringLiteral("json"), queryString);

    QUrl searchUrl;
    searchUrl.setScheme(QLatin1String("baloosearch"));
    searchUrl.setQuery(urlQuery);

    return searchUrl;
}

/**
 * Defines the parameters for the test cases in testBalooSearchParsing()
 */
void DolphinSearchBoxTest::testBalooSearchParsing_data()
{

    QTest::addColumn<QUrl>("searchUrl");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QStringList>("expectedTerms");
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

    const QString tag = QStringLiteral("tag:tagA");
    const QString tagS = QStringLiteral("tag:\"tagB with spaces\"");  // in search url
    const QString tagR = QStringLiteral("tag:tagB with spaces");      // in result term

    // Test for "Content"
    QTest::newRow("content")              << balooQueryUrl(text)   << text  << QStringList() << true  << false;
    QTest::newRow("content/space")        << balooQueryUrl(textS)  << textS << QStringList() << true  << false;
    QTest::newRow("content/quoted")       << balooQueryUrl(textQ)  << textS << QStringList() << true  << false;
    QTest::newRow("content/empty")        << balooQueryUrl("")     << ""    << QStringList() << false << false;
    QTest::newRow("content/single_quote") << balooQueryUrl("\"")   << "\""  << QStringList() << true  << false;
    QTest::newRow("content/double_quote") << balooQueryUrl("\"\"") << ""    << QStringList() << false << false;

    // Test for "FileName"
    QTest::newRow("filename")              << balooQueryUrl(filename)        << text  << QStringList() << false << true;
    QTest::newRow("filename/space")        << balooQueryUrl(filenameS)       << textS << QStringList() << false << true;
    QTest::newRow("filename/quoted")       << balooQueryUrl(filenameQ)       << textQ << QStringList() << false << true;
    QTest::newRow("filename/mixed")        << balooQueryUrl(filenameM)       << textM << QStringList() << false << true;
    QTest::newRow("filename/empty")        << balooQueryUrl("filename:")     << ""    << QStringList() << false << false;
    QTest::newRow("filename/single_quote") << balooQueryUrl("filename:\"")   << "\""  << QStringList() << false << true;
    QTest::newRow("filename/double_quote") << balooQueryUrl("filename:\"\"") << ""    << QStringList() << false << false;

    // Combined content and filename search
    QTest::newRow("content+filename")
        << balooQueryUrl(text + " " + filename)
        << text + " " + filename << QStringList() << true << true;

    QTest::newRow("content+filename/quoted")
        << balooQueryUrl(textQ + " " + filenameQ)
        << textS + " " + filenameQ << QStringList() << true << true;

    // Test for rating
    QTest::newRow("rating")          << balooQueryUrl(rating)                  << ""   << QStringList({rating}) << false << false;
    QTest::newRow("rating+content")  << balooQueryUrl(rating + " " + text)     << text << QStringList({rating}) << true  << false;
    QTest::newRow("rating+filename") << balooQueryUrl(rating + " " + filename) << text << QStringList({rating}) << false << true;

    // Test for modified date
    QTest::newRow("modified")          << balooQueryUrl(modified)                  << ""   << QStringList({modified}) << false << false;
    QTest::newRow("modified+content")  << balooQueryUrl(modified + " " + text)     << text << QStringList({modified}) << true  << false;
    QTest::newRow("modified+filename") << balooQueryUrl(modified + " " + filename) << text << QStringList({modified}) << false << true;

    // Test for tags
    QTest::newRow("tag")          << balooQueryUrl(tag)                  << ""   << QStringList({tag})       << false << false;
    QTest::newRow("tag/space" )   << balooQueryUrl(tagS)                 << ""   << QStringList({tagR})      << false << false;
    QTest::newRow("tag/double")   << balooQueryUrl(tag + " " + tagS)     << ""   << QStringList({tag, tagR}) << false << false;
    QTest::newRow("tag+content")  << balooQueryUrl(tag + " " + text)     << text << QStringList({tag})       << true  << false;
    QTest::newRow("tag+filename") << balooQueryUrl(tag + " " + filename) << text << QStringList({tag})       << false << true;

    // Combined search terms
    QTest::newRow("searchTerms")
        << balooQueryUrl(rating + " AND " + modified + " AND " + tag + " AND " + tagS)
        << "" << QStringList({modified, rating, tag, tagR}) << false << false;

    QTest::newRow("searchTerms+content")
        << balooQueryUrl(rating + " AND " + modified + " " + text + " " + tag + " AND " + tagS)
        << text << QStringList({modified, rating, tag, tagR}) << true << false;

    QTest::newRow("searchTerms+filename")
        << balooQueryUrl(rating + " AND " + modified + " " + filename + " " + tag + " AND " + tagS)
        << text << QStringList({modified, rating, tag, tagR}) << false << true;

    QTest::newRow("allTerms")
        << balooQueryUrl(text + " " + filename + " " + rating + " AND " + modified + " AND " + tag)
        << text + " " + filename << QStringList({modified, rating, tag}) << true << true;

    QTest::newRow("allTerms/space")
        << balooQueryUrl(textS + " " + filenameS + " " + rating + " AND " + modified + " AND " + tagS)
        << textS + " " + filenameS << QStringList({modified, rating, tagR}) << true << true;
}

/**
 * The test verifies whether the different terms search URL (e.g. "baloosearch:/") are
 * properly handled by the searchbox, and only "user" or filename terms are added to the
 * text bar of the searchbox.
 */
void DolphinSearchBoxTest::testBalooSearchParsing()
{
    QFETCH(QUrl, searchUrl);
    QFETCH(QString, expectedText);
    QFETCH(QStringList, expectedTerms);
    QFETCH(bool, hasContent);
    QFETCH(bool, hasFileName);

    const DolphinQuery query = DolphinQuery::fromSearchUrl(searchUrl);

    // Checkt that the URL is supported
    QVERIFY(DolphinQuery::supportsScheme(searchUrl.scheme()));

    // Check for parsed text (would be displayed on the input search bar)
    QCOMPARE(query.text(), expectedText);

    // Check for parsed search terms (would be displayed by the facetsWidget)
    QStringList searchTerms = query.searchTerms();
    searchTerms.sort();

    QCOMPARE(searchTerms.count(), expectedTerms.count());
    for (int i = 0; i < expectedTerms.count(); i++) {
        QCOMPARE(searchTerms.at(i), expectedTerms.at(i));
    }

    // Check for filename and content detection
    QCOMPARE(query.hasContentSearch(), hasContent);
    QCOMPARE(query.hasFileName(), hasFileName);
}

QTEST_MAIN(DolphinSearchBoxTest)

#include "dolphinquerytest.moc"
