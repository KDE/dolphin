/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "search/bar.h"
#include "search/popup.h"

#include <QLineEdit>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QToolButton>

class DolphinSearchBarTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testPopupLazyLoading();
    void testTextClearing();
    void testUrlChangeSignals();

private:
    Search::Bar *m_searchBar;
};

void DolphinSearchBarTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void DolphinSearchBarTest::init()
{
    const auto homeUrl{QUrl::fromUserInput(QDir::homePath())};
    m_searchBar = new Search::Bar(std::make_shared<const Search::DolphinQuery>(homeUrl, homeUrl));
}

void DolphinSearchBarTest::cleanup()
{
    delete m_searchBar;
}

void DolphinSearchBarTest::testPopupLazyLoading()
{
    m_searchBar->setVisible(true, WithoutAnimation);
    QVERIFY2(m_searchBar->m_popup->isEmpty(), "The popup should only be populated or updated when it was opened at least once by the user.");
}

/**
 * The test verifies whether the automatic clearing of the text works correctly.
 * The text may not get cleared when the search bar gets visible or invisible,
 * as this would clear the text when switching between tabs.
 */
void DolphinSearchBarTest::testTextClearing()
{
    m_searchBar->setVisible(true, WithoutAnimation);
    QVERIFY(m_searchBar->text().isEmpty());
    QVERIFY(!m_searchBar->isSearchConfigured());

    m_searchBar->updateStateToMatch(std::make_shared<const Search::DolphinQuery>(QUrl::fromUserInput("filenamesearch:?search=xyz"), QUrl{}));
    m_searchBar->setVisible(false, WithoutAnimation);
    m_searchBar->setVisible(true, WithoutAnimation);
    QCOMPARE(m_searchBar->text(), QStringLiteral("xyz"));
    QVERIFY(m_searchBar->isSearchConfigured());
    QVERIFY(!m_searchBar->queryTitle().isEmpty());

    QTest::keyClick(m_searchBar, Qt::Key_Escape);
    QVERIFY(m_searchBar->text().isEmpty());
    QVERIFY(!m_searchBar->isSearchConfigured());
}

void DolphinSearchBarTest::testUrlChangeSignals()
{
    QSignalSpy spyUrlChangeRequested(m_searchBar, &Search::Bar::urlChangeRequested);
    m_searchBar->setVisible(true, WithoutAnimation);
    QVERIFY2(spyUrlChangeRequested.isEmpty(), "Opening the Search::Bar should not trigger anything.");

    m_searchBar->m_everywhereButton->click();
    m_searchBar->m_fromHereButton->click();
    QVERIFY(!m_searchBar->isSearchConfigured());

    while (!spyUrlChangeRequested.isEmpty()) {
        const QUrl requestedUrl = qvariant_cast<QUrl>(spyUrlChangeRequested.takeFirst().at(0));
        QVERIFY2(!Search::isSupportedSearchScheme(requestedUrl.scheme()) && requestedUrl == m_searchBar->m_searchConfiguration->searchPath(),
                 "The search is still not in a state to search for anything specific, so any requested URLs would be identical to the current search path of "
                 "the Search::Bar.");
    }

    Search::DolphinQuery searchConfiguration = *m_searchBar->m_searchConfiguration;
    searchConfiguration.setSearchTerm(QStringLiteral("searchTerm"));
    m_searchBar->updateStateToMatch(std::make_shared<const Search::DolphinQuery>(searchConfiguration));
    QVERIFY2(m_searchBar->isSearchConfigured(), "The search bar now has enough information to trigger a meaningful search.");
    QVERIFY2(spyUrlChangeRequested.isEmpty(), "The visual state was updated to match a new search configuration, but the user never triggered a search.");

    m_searchBar->commitCurrentConfiguration(); // We pretend to be a user interaction that would trigger a search to happen.
    const QUrl requestedUrl = qvariant_cast<QUrl>(spyUrlChangeRequested.takeFirst().at(0));
    QVERIFY2(Search::isSupportedSearchScheme(requestedUrl.scheme()), "The search bar requested to open a search url and therefore start searching.");
    QVERIFY2(spyUrlChangeRequested.isEmpty(), "The search URL was requested exactly once, so no additional urlChangeRequested signals should exist.");

    Search::DolphinQuery searchConfiguration2 = *m_searchBar->m_searchConfiguration;
    searchConfiguration2.setSearchTerm(QString(""));
    m_searchBar->updateStateToMatch(std::make_shared<const Search::DolphinQuery>(searchConfiguration2));
    QVERIFY2(!m_searchBar->isSearchConfigured(), "The search bar does not have enough information anymore to trigger a meaningful search.");
    QVERIFY2(spyUrlChangeRequested.isEmpty(), "The visual state was updated to match a new search configuration, but the user never triggered a search.");

    m_searchBar->commitCurrentConfiguration(); // We pretend to be a user interaction that would trigger a search to happen.
    const QUrl requestedUrl2 = qvariant_cast<QUrl>(spyUrlChangeRequested.takeFirst().at(0));
    QVERIFY2(!Search::isSupportedSearchScheme(requestedUrl2.scheme()) && requestedUrl2 == m_searchBar->m_searchConfiguration->searchPath(),
             "The Search::Bar is not in a state to search for anything specific, so the search bar requests to show the previously visited location normally "
             "again instead of any previous search URL.");
    QVERIFY2(spyUrlChangeRequested.isEmpty(), "The non-search URL was requested exactly once, so no additional urlChangeRequested signals should exist.");

    QVERIFY2(m_searchBar->m_popup->isEmpty(), "Through all of this, the popup should still be empty because it was never opened by the user.");
}

QTEST_MAIN(DolphinSearchBarTest)

#include "dolphinsearchbartest.moc"
