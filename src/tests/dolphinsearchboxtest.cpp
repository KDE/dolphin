/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "search/dolphinsearchbox.h"

#include <QTest>

class DolphinSearchBoxTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void testTextClearing();

private:
    DolphinSearchBox* m_searchBox;
};

void DolphinSearchBoxTest::init()
{
    m_searchBox = new DolphinSearchBox();
}

void DolphinSearchBoxTest::cleanup()
{
    delete m_searchBox;
}

/**
 * The test verifies whether the automatic clearing of the text works correctly.
 * The text may not get cleared when the searchbox gets visible or invisible,
 * as this would clear the text when switching between tabs.
 */
void DolphinSearchBoxTest::testTextClearing()
{
    m_searchBox->show();
    QVERIFY(m_searchBox->text().isEmpty());

    m_searchBox->setText("xyz");
    m_searchBox->hide();
    m_searchBox->show();
    QCOMPARE(m_searchBox->text(), QString("xyz"));

    QTest::keyClick(m_searchBox, Qt::Key_Escape);
    QVERIFY(m_searchBox->text().isEmpty());
}

QTEST_MAIN(DolphinSearchBoxTest)

#include "dolphinsearchboxtest.moc"
