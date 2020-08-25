/*
 * SPDX-FileCopyrightText: 2017 Emirald Mateli <aldo.mateli@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
