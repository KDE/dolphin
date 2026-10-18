/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "panels/folders/folderspanel.h"
#include "dolphin_folderspanelsettings.h"
#include "dolphin_generalsettings.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kitemlistview.h"

#include "testdir.h"

#include <QDir>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class FoldersPanelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testCurrentFolderIsSelected();
    void testCurrentFolderNameIsShown();
    void testNameWiderThanThePanelIsShownFromItsStart();
    void testLimitToHomeFollowsTheSettings();

private:
    /** Where the name of the item at @p index ends, indentation and icon
     * included. */
    static qreal nameEnd(const KItemListView *view, int index);
    /** Where that name begins. */
    static qreal nameStart(const KItemListView *view, const KFileItemModel *model, int index);
};

void FoldersPanelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

qreal FoldersPanelTest::nameEnd(const KItemListView *view, int index)
{
    return view->widgetCreator()->preferredRoleColumnWidth("text", index, view);
}

qreal FoldersPanelTest::nameStart(const KItemListView *view, const KFileItemModel *model, int index)
{
    return nameEnd(view, index) - view->styleOption().fontMetrics.horizontalAdvance(model->fileItem(index).text());
}

namespace
{
/** A tree of @p levels nested folders, each named @p name plus its level, and
 * the deepest url. */
QUrl createNestedFolders(TestDir &testDir, const QString &name, int levels)
{
    QString path;
    for (int level = 0; level < levels; ++level) {
        path += QStringLiteral("%1-%2/").arg(name).arg(level);
    }
    testDir.createDir(path);
    return QUrl::fromLocalFile(testDir.path() + QLatin1Char('/') + path);
}

/** Shows the panel, points it at @p url and hands back the container it has
 * built. */
KItemListContainer *showPanelAt(FoldersPanel &panel, const QUrl &url)
{
    panel.resize(200, 400);
    panel.show();
    if (!QTest::qWaitForWindowExposed(&panel)) {
        return nullptr;
    }
    panel.setUrl(url);
    return panel.findChild<KItemListContainer *>();
}
} // namespace

void FoldersPanelTest::testCurrentFolderIsSelected()
{
    // The tree reaches a folder several levels down one loading at a time, and
    // each of them completes. The panel selects the folder once it is there.
    TestDir testDir;
    const QUrl deepUrl = createNestedFolders(testDir, QStringLiteral("folder"), 8);

    FoldersPanel panel;
    KItemListContainer *container = showPanelAt(panel, deepUrl);
    QVERIFY(container);
    auto *model = static_cast<KFileItemModel *>(container->controller()->model());

    QTRY_VERIFY_WITH_TIMEOUT(model->index(deepUrl) >= 0, 10000);
    QTRY_COMPARE(container->controller()->selectionManager()->currentItem(), model->index(deepUrl));
}

void FoldersPanelTest::testCurrentFolderNameIsShown()
{
    // Every item of the tree is as wide as the widest one, so the name of a
    // folder deep in the hierarchy sits outside a narrow panel until the panel
    // scrolls to it.
    TestDir testDir;
    const QUrl deepUrl = createNestedFolders(testDir, QStringLiteral("folder"), 8);

    FoldersPanel panel;
    KItemListContainer *container = showPanelAt(panel, deepUrl);
    QVERIFY(container);
    KItemListView *view = container->controller()->view();
    auto *model = static_cast<KFileItemModel *>(container->controller()->model());

    QTRY_VERIFY_WITH_TIMEOUT(model->index(deepUrl) >= 0, 10000);
    const int index = model->index(deepUrl);

    // There is something to scroll: the name ends beyond the right edge of the
    // panel.
    QTRY_VERIFY(nameEnd(view, index) > view->size().width());

    QTRY_VERIFY(view->itemOffset() > 0);
    QTRY_VERIFY(view->itemOffset() + view->size().width() >= nameEnd(view, index));
    QVERIFY(view->itemOffset() <= nameStart(view, model, index));

    // The scroll bar says where the view now is, so dragging it carries on from there.
    QTRY_COMPARE(container->horizontalScrollBar()->value(), int(view->itemOffset()));

    // Scrolling sideways leaves the scrolling up and down to the item as it was.
    QTRY_VERIFY(view->itemRect(index).top() >= 0);
    QTRY_VERIFY(view->itemRect(index).bottom() <= view->size().height());

    // Going back to the top of the tree shows the name there, which means
    // scrolling the other way.
    panel.setUrl(testDir.url());
    QTRY_VERIFY(model->index(testDir.url()) >= 0);
    const int topIndex = model->index(testDir.url());
    QTRY_VERIFY(view->itemOffset() <= nameStart(view, model, topIndex));
}

void FoldersPanelTest::testNameWiderThanThePanelIsShownFromItsStart()
{
    TestDir testDir;
    const QUrl deepUrl = createNestedFolders(testDir, QStringLiteral("a-folder-with-quite-a-long-name"), 5);

    FoldersPanel panel;
    KItemListContainer *container = showPanelAt(panel, deepUrl);
    QVERIFY(container);
    KItemListView *view = container->controller()->view();
    auto *model = static_cast<KFileItemModel *>(container->controller()->model());

    QTRY_VERIFY_WITH_TIMEOUT(model->index(deepUrl) >= 0, 10000);
    const int index = model->index(deepUrl);

    QVERIFY(nameEnd(view, index) - nameStart(view, model, index) > view->size().width());

    // The end of the name cannot be shown together with its beginning, and the
    // beginning is the half a reader needs.
    QTRY_COMPARE(view->itemOffset(), nameStart(view, model, index));
}

void FoldersPanelTest::testLimitToHomeFollowsTheSettings()
{
    // The tree starts at the home folder or at the root folder, following the setting, and
    // the folder shown in the view stays selected when the setting changes.
    TestDir testDir;
    const QUrl deepUrl = createNestedFolders(testDir, QStringLiteral("folder"), 3);
    const QUrl rootUrl = QUrl::fromLocalFile(QDir::rootPath());
    GeneralSettings::setHomeUrl(testDir.path());
    FoldersPanelSettings::setLimitFoldersPanelToHome(true);

    FoldersPanel panel;
    QSignalSpy folderActivatedSpy(&panel, &FoldersPanel::folderActivated);
    KItemListContainer *container = showPanelAt(panel, deepUrl);
    QVERIFY(container);
    auto *model = static_cast<KFileItemModel *>(container->controller()->model());
    QTRY_VERIFY_WITH_TIMEOUT(model->index(deepUrl) >= 0, 10000);
    QCOMPARE(model->directory(), testDir.url());

    FoldersPanelSettings::setLimitFoldersPanelToHome(false);
    panel.readSettings();
    QTRY_COMPARE(model->directory(), rootUrl);
    QTRY_VERIFY_WITH_TIMEOUT(model->index(testDir.url()) >= 0, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(model->index(deepUrl) >= 0, 10000);
    QTRY_COMPARE(container->controller()->selectionManager()->currentItem(), model->index(deepUrl));

    FoldersPanelSettings::setLimitFoldersPanelToHome(true);
    panel.readSettings();
    QTRY_COMPARE(model->directory(), testDir.url());
    QTRY_VERIFY_WITH_TIMEOUT(model->index(deepUrl) >= 0, 10000);

    // Outside the home folder the tree starts at the root folder, and the view stays where it is.
    const QUrl outsideHomeUrl = QUrl::fromLocalFile(QDir::tempPath());
    panel.setUrl(outsideHomeUrl);
    QTRY_COMPARE(model->directory(), rootUrl);
    panel.readSettings();
    QCOMPARE(model->directory(), rootUrl);
    QCOMPARE(folderActivatedSpy.count(), 0);

    GeneralSettings::self()->setDefaults();
    FoldersPanelSettings::self()->setDefaults();
}

QTEST_MAIN(FoldersPanelTest)

#include "folderspaneltest.moc"
