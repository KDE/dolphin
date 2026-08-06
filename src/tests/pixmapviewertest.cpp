/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "panels/information/pixmapviewer.h"

#include <QPainter>
#include <QTest>

// A pixmap of the given size, white but for a red corner at the top left and a blue one at the bottom
// right, so that what became of its corners says how it was drawn.
static QPixmap cornerMarkedPixmap(int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::white);

    const int corner = size / 10;
    QPainter painter(&pixmap);
    painter.fillRect(0, 0, corner, corner, Qt::red);
    painter.fillRect(size - corner, size - corner, corner, corner, Qt::blue);
    painter.end();
    return pixmap;
}

static QImage renderOf(PixmapViewer &viewer)
{
    QImage image(viewer.size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    viewer.render(&image);
    return image;
}

class PixmapViewerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testPixmapLargerThanTheViewerIsDrawnToFit();
    void testPixmapSmallerThanTheViewerKeepsItsSize();
};

// A preview made for a panel wider than the one it ends up in is drawn to fit, so all of it is seen.
void PixmapViewerTest::testPixmapLargerThanTheViewerIsDrawnToFit()
{
    PixmapViewer viewer(nullptr);
    viewer.resize(300, 300);
    // The viewer holds a minimum size of its own, so what it ended up with is what the corners are read
    // against.
    const QSize shown = viewer.size();
    viewer.setPixmap(cornerMarkedPixmap(qMax(shown.width(), shown.height()) * 3));

    const QImage image = renderOf(viewer);
    QCOMPARE(image.pixelColor(2, 2), QColor(Qt::red));
    QCOMPARE(image.pixelColor(shown.width() - 3, shown.height() - 3), QColor(Qt::blue));
}

// One that fits is left as it is, rather than being blown up to the panel.
void PixmapViewerTest::testPixmapSmallerThanTheViewerKeepsItsSize()
{
    PixmapViewer viewer(nullptr);
    viewer.resize(400, 400);
    const QSize shown = viewer.size();
    const int pixmapSize = 100;
    viewer.setPixmap(cornerMarkedPixmap(pixmapSize));

    const QImage image = renderOf(viewer);
    // The pixmap is centred at its own size, so its corners are inside the viewer rather than at the
    // corners of it, which are none of the pixmap.
    const int left = (shown.width() - pixmapSize) / 2;
    const int top = (shown.height() - pixmapSize) / 2;
    QCOMPARE(image.pixelColor(left + 2, top + 2), QColor(Qt::red));
    QCOMPARE(image.pixelColor(left + pixmapSize - 3, top + pixmapSize - 3), QColor(Qt::blue));
    QVERIFY(image.pixelColor(2, 2) != QColor(Qt::red));
    QVERIFY(image.pixelColor(shown.width() - 3, shown.height() - 3) != QColor(Qt::blue));
}

QTEST_MAIN(PixmapViewerTest)

#include "pixmapviewertest.moc"
