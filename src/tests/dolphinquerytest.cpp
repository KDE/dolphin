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
 * Defines the parameters for the test cases in testBalooSearchParsing()
 */
void DolphinSearchBoxTest::testBalooSearchParsing_data()
{
    const QString text = QStringLiteral("abc xyz");
    const QString filename = QStringLiteral("filename:\"%1\"").arg(text);
    const QString rating = QStringLiteral("rating>=2");
    const QString modified = QString("modified>=2019-08-07");
    const QString tagA = QString("tag:tagA");
    const QString tagB = QString("tag:tagB");

    QTest::addColumn<QString>("searchString");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QStringList>("expectedTerms");
    QTest::addColumn<bool>("hasContent");
    QTest::addColumn<bool>("hasFileName");

    // Test for "Content"
    QTest::newRow("content")              << text    << text << QStringList() << true  << false;
    QTest::newRow("content/empty")        << ""      << ""   << QStringList() << false << false;
    QTest::newRow("content/singleQuote")  << "\""    << ""   << QStringList() << false << false;
    QTest::newRow("content/doubleQuote")  << "\"\""  << ""   << QStringList() << false << false;

    // Test for "FileName"
    QTest::newRow("filename")             << filename         << text << QStringList() << false << true;
    QTest::newRow("filename/empty")       << "filename:"      << ""   << QStringList() << false << false;
    QTest::newRow("filename/singleQuote") << "filename:\""    << ""   << QStringList() << false << false;
    QTest::newRow("filename/doubleQuote") << "filename:\"\""  << ""   << QStringList() << false << false;

    // Combined content and filename search
    QTest::newRow("content+filename") << text + " " + filename << text + " " + filename << QStringList() << true << true;

    // Test for rating
    QTest::newRow("rating")          << rating                  << ""   << QStringList({rating}) << false << false;
    QTest::newRow("rating+content")  << rating + " " + text     << text << QStringList({rating}) << true  << false;
    QTest::newRow("rating+filename") << rating + " " + filename << text << QStringList({rating}) << false << true;

    // Test for modified date
    QTest::newRow("modified")          << modified                  << ""   << QStringList({modified}) << false << false;
    QTest::newRow("modified+content")  << modified + " " + text     << text << QStringList({modified}) << true  << false;
    QTest::newRow("modified+filename") << modified + " " + filename << text << QStringList({modified}) << false << true;

    // Test for tags
    QTest::newRow("tag")          << tagA                  << ""   << QStringList({tagA})       << false << false;
    QTest::newRow("tag/double")   << tagA + " " + tagB     << ""   << QStringList({tagA, tagB}) << false << false;
    QTest::newRow("tag+content")  << tagA + " " + text     << text << QStringList({tagA})       << true  << false;
    QTest::newRow("tag+filename") << tagA + " " + filename << text << QStringList({tagA})       << false << true;

    // Combined search terms
    QTest::newRow("allTerms")
        << rating + " AND " + modified + " AND " + tagA + " AND " + tagB
        << "" << QStringList({modified, rating, tagA, tagB}) << false << false;

    QTest::newRow("allTerms+content")
        << rating + " AND " + modified + " " + text + " " + tagA + " AND " + tagB
        << text << QStringList({modified, rating, tagA, tagB}) << true << false;

    QTest::newRow("allTerms+filename")
        << rating + " AND " + modified + " " + filename + " " + tagA + " AND " + tagB
        << text << QStringList({modified, rating, tagA, tagB}) << false << true;

    QTest::newRow("allTerms+content+filename")
        << text + " " + filename + " " + rating + " AND " + modified + " AND " + tagA + " AND " + tagB
        << text + " " + filename << QStringList({modified, rating, tagA, tagB}) << true << true;
}

/**
 * Helper function to compose the baloo query URL used for searching
 */
QUrl composeQueryUrl(const QString& searchString)
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
 * The test verifies whether the different terms of a Baloo search URL ("baloosearch:") are
 * properly handled by the searchbox, and only "user" or filename terms are added to the
 * text bar of the searchbox.
 */
void DolphinSearchBoxTest::testBalooSearchParsing()
{
    QFETCH(QString, searchString);
    QFETCH(QString, expectedText);
    QFETCH(QStringList, expectedTerms);
    QFETCH(bool, hasContent);
    QFETCH(bool, hasFileName);

    const QUrl testUrl = composeQueryUrl(searchString);
    const DolphinQuery query = DolphinQuery::fromBalooSearchUrl(testUrl);

    QStringList searchTerms = query.searchTerms();
    searchTerms.sort();

    // Check for parsed text (would be displayed on the input search bar)
    QCOMPARE(query.text(), expectedText);

    // Check for parsed search terms (would be displayed by the facetsWidget)
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
