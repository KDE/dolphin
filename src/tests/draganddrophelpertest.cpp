/***************************************************************************
 *   Copyright (C) 2017 by Emirald Mateli <aldo.mateli@gmail.com>          *
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

#include <QTest>
#include <views/draganddrophelper.h>

class DragAndDropHelperTest : public QObject
{
    Q_OBJECT

private slots:
    void testUrlListMatchesUrl_data();
    void testUrlListMatchesUrl();
};

void DragAndDropHelperTest::testUrlListMatchesUrl_data()
{
    QTest::addColumn<QList<QUrl>>("urlList");
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("expected");

    QTest::newRow("test_equal")
            << QList<QUrl> {QUrl::fromLocalFile("/root")}
            << QUrl::fromLocalFile("/root")
            << true;

    QTest::newRow("test_trailing_slash")
            << QList<QUrl> {QUrl::fromLocalFile("/root/")}
            << QUrl::fromLocalFile("/root")
            << true;

    QTest::newRow("test_ftp_scheme")
            << QList<QUrl> {QUrl("ftp://server:2211/dir")}
            << QUrl("ftp://server:2211/dir")
            << true;

    QTest::newRow("test_not_matched")
            << QList<QUrl> {QUrl::fromLocalFile("/usr/share"), QUrl::fromLocalFile("/usr/local/bin")}
            << QUrl::fromLocalFile("/usr/bin")
            << false;

    QTest::newRow("test_empty_target")
            << QList<QUrl> {QUrl::fromLocalFile("/usr/share"), QUrl::fromLocalFile("/usr/local/bin")}
            << QUrl()
            << false;

    QTest::newRow("test_empty_list")
            << QList<QUrl>()
            << QUrl::fromLocalFile("/usr/bin")
            << false;
}

void DragAndDropHelperTest::testUrlListMatchesUrl()
{
    QFETCH(QList<QUrl>, urlList);
    QFETCH(QUrl, url);
    QFETCH(bool, expected);

    QCOMPARE(DragAndDropHelper::urlListMatchesUrl(urlList, url), expected);
}


QTEST_MAIN(DragAndDropHelperTest)

#include "draganddrophelpertest.moc"
