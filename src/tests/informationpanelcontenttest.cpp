/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "panels/information/informationpanelcontent.h"
#include "panels/information/pixmapviewer.h"

#include <KFileItem>

#include <QPainter>
#include <QTemporaryDir>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

class InformationPanelContentTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testThePreviewGrowsWithThePanel();

private:
    QTemporaryDir m_dir;
    QString m_imagePath;
};

void InformationPanelContentTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
    m_imagePath = m_dir.path() + QLatin1String("/big.png");

    // Larger than any panel the test gives it, so what arrives is what was asked for rather than all the
    // image has.
    QImage image(2000, 2000, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.fillRect(0, 0, 1000, 1000, Qt::red);
    painter.end();
    QVERIFY(image.save(m_imagePath));
}

// A panel that is made wider shows a preview made for the room it has now, rather than the one it was
// given when the file was selected.
void InformationPanelContentTest::testThePreviewGrowsWithThePanel()
{
    // The panel builds its content when it is shown, so the content is built here as it is there, while
    // the panel it is given is on its way to the screen.
    QWidget panel;
    panel.resize(300, 900);
    panel.show();
    QVERIFY(QTest::qWaitForWindowExposed(&panel));

    auto *content = new InformationPanelContent(&panel);
    QVBoxLayout *layout = new QVBoxLayout(&panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(content);

    auto *viewer = content->findChild<PixmapViewer *>();
    QVERIFY(viewer);

    content->showItem(KFileItem(QUrl::fromLocalFile(m_imagePath)));

    // Every preview is made by a thumbnailer of the system, which the test cannot do without.
    if (!QTest::qWaitFor([viewer]() {
            return !viewer->pixmap().isNull();
        })) {
        QSKIP("no thumbnailer answered for a png");
    }

    const int narrow = viewer->pixmap().width();
    QVERIFY(narrow > 0);

    panel.resize(900, 900);
    QTRY_VERIFY(viewer->pixmap().width() > narrow);

    // What arrived is of the size the viewer grew to, not merely bigger than it was: it fits the viewer,
    // and fills it in the direction that bounds it.
    const QSize preview = viewer->pixmap().deviceIndependentSize().toSize();
    QVERIFY(preview.width() <= viewer->width());
    QVERIFY(preview.height() <= viewer->height());
    QCOMPARE(qMax(preview.width(), preview.height()), qMin(viewer->width(), viewer->height()));
}

QTEST_MAIN(InformationPanelContentTest)

#include "informationpanelcontenttest.moc"
