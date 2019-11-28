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
    const QString text = QStringLiteral("xyz");
    const QString filename = QStringLiteral("filename:\"xyz\"");
    const QString rating = QStringLiteral("rating>=2");
    const QString modified = QString("modified>=2019-08-07");

    QTest::addColumn<QString>("searchString");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QStringList>("expectedTerms");

    // Test for "Content"
    QTest::newRow("content")              << text    << text << QStringList();
    QTest::newRow("content/empty")        << ""      << ""   << QStringList();
    QTest::newRow("content/singleQuote")  << "\""    << ""   << QStringList();
    QTest::newRow("content/doubleQuote")  << "\"\""  << ""   << QStringList();
    // Test for empty `filename`
    QTest::newRow("filename")             << filename         << text << QStringList();
    QTest::newRow("filename/empty")       << "filename:"      << ""   << QStringList();
    QTest::newRow("filename/singleQuote") << "filename:\""    << ""   << QStringList();
    QTest::newRow("filename/doubleQuote") << "filename:\"\""  << ""   << QStringList();

    // Test for rating
    QTest::newRow("rating")          << rating                  << ""   << QStringList({rating});
    QTest::newRow("rating+content")  << rating + " " + text     << text << QStringList({rating});
    QTest::newRow("rating+filename") << rating + " " + filename << text << QStringList({rating});
    // Test for modified date
    QTest::newRow("modified")          << modified                  << ""   << QStringList({modified});
    QTest::newRow("modified+content")  << modified + " " + text     << text << QStringList({modified});
    QTest::newRow("modified+filename") << modified + " " + filename << text << QStringList({modified});
    // Combined tests
    QTest::newRow("rating+modified")          << rating + " AND " + modified                  << ""   << QStringList({modified, rating});
    QTest::newRow("rating+modified+content")  << rating + " AND " + modified + " " + text     << text << QStringList({modified, rating});
    QTest::newRow("rating+modified+filename") << rating + " AND " + modified + " " + filename << text << QStringList({modified, rating});
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
}

QTEST_MAIN(DolphinSearchBoxTest)

#include "dolphinquerytest.moc"
