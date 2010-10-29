/***************************************************************************
 *   Copyright (C) 2010 by Frank Reininghaus (frank78ac@googlemail.com)    *
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

#include "testbase.h"

#include "views/dolphindetailsview.h"
#include "views/dolphinview.h"
#include "views/dolphinmodel.h"
#include "views/dolphindirlister.h"
#include "views/dolphinsortfilterproxymodel.h"
#include "views/zoomlevelinfo.h"

#include <KTempDir>

#include <QtCore/QDir>

#include "kdebug.h"

class DolphinDetailsViewTest : public TestBase
{
    Q_OBJECT

private slots:

    void initTestCase();

    void bug234600_overlappingIconsWhenZooming();

};

void DolphinDetailsViewTest::initTestCase()
{
    // add time stamps to find origin of test failures due to timeout at
    // http://my.cdash.org/index.php?project=kdebase&date=
    qputenv("KDE_DEBUG_TIMESTAMP", QByteArray("1"));
}

/**
 * When the icon size is changed, we have to make sure that the maximumSize given
 * to KFileItemDelegate for rendering each item is updated correctly. If this is not
 * done, the visualRects are clipped by the incorrect maximum size, and the icons
 * may overlap, see
 *
 * https://bugs.kde.org/show_bug.cgi?id=234600
 */

void DolphinDetailsViewTest::bug234600_overlappingIconsWhenZooming()
{
    QStringList files;
    files << "a" << "b" << "c" << "d";
    createFiles(files);

    m_view->setMode(DolphinView::DetailsView);
    DolphinDetailsView* detailsView = qobject_cast<DolphinDetailsView*>(itemView());
    QVERIFY(detailsView);
    m_view->resize(400, 400);
    m_view->show();
    QTest::qWaitForWindowShown(m_view);

    // We have to make sure that the view has loaded the directory before we start the test.
    // TODO: This will be needed frequently. Maybe move to TestHelper.
    kDebug() << "Reloading view and waiting for the finishedPathLoading(const KUrl&) signal...";
    QSignalSpy finished(m_view, SIGNAL(finishedPathLoading(const KUrl&)));
    m_view->reload();
    while (finished.count() != 1) {
        QTest::qWait(50);
    }
    kDebug() << "...signal received, continuing";

    QModelIndex index0 = detailsView->model()->index(0, 0);
    detailsView->setCurrentIndex(index0);
    QCOMPARE(detailsView->currentIndex(), index0);

    // Setting the zoom level to the minimum value and triggering DolphinDetailsView::currentChanged(...)
    // should make sure that the bug is triggered.
    int zoomLevel = ZoomLevelInfo::minimumLevel();
    m_view->setZoomLevel(zoomLevel);

    QModelIndex index1 = detailsView->model()->index(1, 0);
    detailsView->setCurrentIndex(index1);
    QCOMPARE(detailsView->currentIndex(), index1);

    kDebug() << "Now checking zoom levels...";

    // Increase the zoom level successively to the maximum.
    while(zoomLevel < ZoomLevelInfo::maximumLevel()) {
        zoomLevel++;
        kDebug() << "Testing zoom level" << zoomLevel;
        m_view->setZoomLevel(zoomLevel);

        //Check for each zoom level that the height of each item is at least the icon size.
        QVERIFY(detailsView->visualRect(index1).height() >= ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel));
    }

    m_view->hide();

    kDebug() << "Cleaning up test directory...";
    cleanupTestDir();
    kDebug() << "Done.";
}

QTEST_KDEMAIN(DolphinDetailsViewTest, GUI)

#include "dolphindetailsviewtest.moc"