/*
 * SPDX-FileCopyrightText: 2026 Meven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "views/dolphinview.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "kitemviews/kitemlistcontainer.h"
#include "testdir.h"
#include "views/viewproperties.h"
#include "views/zoomlevelinfo.h"

#include <KIconLoader>

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class DolphinViewTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void selectionIsAnnouncedBeforeTheBackgroundContextMenu();
    void defaultZoomLevelComesFromThePreviewSizeWhenPreviewsAreShown();

private:
    void requestBackgroundContextMenu();

    TestDir *m_testDir = nullptr;
    DolphinView *m_view = nullptr;
};

void DolphinViewTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void DolphinViewTest::init()
{
    m_testDir = new TestDir();
    m_testDir->createFile(QStringLiteral("a.txt"));
    m_testDir->createFile(QStringLiteral("b.txt"));

    m_view = new DolphinView(m_testDir->url(), nullptr);
    m_view->resize(400, 400);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));
    QTRY_COMPARE(m_view->itemsCount(), 2);
}

void DolphinViewTest::cleanup()
{
    delete m_view;
    m_view = nullptr;
    delete m_testDir;
    m_testDir = nullptr;
}

/**
 * A context menu asked for on the background of the view comes with the selection emptied by the
 * same click, and whoever fills that menu learns about the empty selection before the menu itself
 * is asked for. See Bug 522372.
 */
void DolphinViewTest::selectionIsAnnouncedBeforeTheBackgroundContextMenu()
{
    QSignalSpy selectionChanged(m_view, &DolphinView::selectionChanged);
    m_view->selectAll();
    QTRY_COMPARE(selectionChanged.count(), 1);
    QCOMPARE(selectionChanged.first().first().value<KFileItemList>().count(), 2);

    QStringList signalsInOrder;
    connect(m_view, &DolphinView::selectionChanged, this, [&signalsInOrder] {
        signalsInOrder << QStringLiteral("selectionChanged");
    });
    connect(m_view, &DolphinView::requestContextMenu, this, [&signalsInOrder] {
        signalsInOrder << QStringLiteral("requestContextMenu");
    });

    m_view->clearSelection();
    requestBackgroundContextMenu();

    QCOMPARE(signalsInOrder, QStringList({QStringLiteral("selectionChanged"), QStringLiteral("requestContextMenu")}));
    QCOMPARE(selectionChanged.last().first().value<KFileItemList>().count(), 0);
}

/**
 * A folder that carries no zoom level of its own falls back to the default one, and while previews
 * are shown that default has to come from the configured preview size rather than the icon size.
 * Otherwise the preview size never reaches such a folder and its slider looks like it does nothing.
 * See Bug 524532.
 */
void DolphinViewTest::defaultZoomLevelComesFromThePreviewSizeWhenPreviewsAreShown()
{
    const bool globalViewProps = GeneralSettings::globalViewProps();
    const int iconSize = IconsModeSettings::iconSize();
    const int previewSize = IconsModeSettings::previewSize();
    auto restoreSettings = qScopeGuard([globalViewProps, iconSize, previewSize] {
        GeneralSettings::setGlobalViewProps(globalViewProps);
        IconsModeSettings::setIconSize(iconSize);
        IconsModeSettings::setPreviewSize(previewSize);
    });

    // Only per-folder view properties have a folder without a zoom level to fall back for.
    GeneralSettings::setGlobalViewProps(false);
    IconsModeSettings::setIconSize(KIconLoader::SizeMedium);
    IconsModeSettings::setPreviewSize(KIconLoader::SizeEnormous);

    const int iconZoomLevel = ZoomLevelInfo::zoomLevelForIconSize(QSize(KIconLoader::SizeMedium, KIconLoader::SizeMedium));
    const int previewZoomLevel = ZoomLevelInfo::zoomLevelForIconSize(QSize(KIconLoader::SizeEnormous, KIconLoader::SizeEnormous));
    QVERIFY(iconZoomLevel != previewZoomLevel);

    // A folder of its own, so that nothing has ever stored a zoom level for it.
    m_testDir->createDir(QStringLiteral("fresh"));
    const QUrl freshFolder = QUrl::fromLocalFile(QDir(m_testDir->path()).filePath(QStringLiteral("fresh")));
    QCOMPARE(ViewProperties(freshFolder).zoomLevel(), -1);

    {
        DolphinView view(freshFolder, nullptr);
        QVERIFY(view.previewsShown());
        QCOMPARE(view.zoomLevel(), previewZoomLevel);
    }

    // The same folder with previews turned off has to fall back to the icon size instead.
    {
        ViewProperties props(freshFolder);
        props.setPreviewsShown(false);
        props.save();
        QCOMPARE(props.zoomLevel(), -1);
    }

    DolphinView view(freshFolder, nullptr);
    QVERIFY(!view.previewsShown());
    QCOMPARE(view.zoomLevel(), iconZoomLevel);
}

void DolphinViewTest::requestBackgroundContextMenu()
{
    KItemListContainer *container = m_view->findChild<KItemListContainer *>();
    QVERIFY(container);

    QContextMenuEvent event(QContextMenuEvent::Keyboard, QPoint(), QPoint());
    QApplication::sendEvent(container, &event);
}

QTEST_MAIN(DolphinViewTest)

#include "dolphinviewtest.moc"
