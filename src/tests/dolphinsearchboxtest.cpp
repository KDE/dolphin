/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include <qtest_kde.h>

#include "search/dolphinsearchbox.h"
#include <qtestkeyboard.h>

class DolphinSearchBoxTest : public QObject
{
    Q_OBJECT

private slots:
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

QTEST_KDEMAIN(DolphinSearchBoxTest, GUI)

#include "dolphinsearchboxtest.moc"
