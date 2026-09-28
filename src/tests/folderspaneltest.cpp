/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "panels/folders/folderspanel.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"

#include "testdir.h"

#include <QStandardPaths>
#include <QTest>

class FoldersPanelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testCurrentFolderIsSelected();
};

void FoldersPanelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

namespace
{
/** A tree of @p levels nested folders, each named @p name plus its level, and the deepest url. */
QUrl createNestedFolders(TestDir &testDir, const QString &name, int levels)
{
    QString path;
    for (int level = 0; level < levels; ++level) {
        path += QStringLiteral("%1-%2/").arg(name).arg(level);
    }
    testDir.createDir(path);
    return QUrl::fromLocalFile(testDir.path() + QLatin1Char('/') + path);
}

/** Shows the panel, points it at @p url and hands back the container it has built. */
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
}

void FoldersPanelTest::testCurrentFolderIsSelected()
{
    // The tree reaches a folder several levels down one loading at a time, and each of them
    // completes. The panel selects the folder once it is there.
    TestDir testDir;
    const QUrl deepUrl = createNestedFolders(testDir, QStringLiteral("folder"), 8);

    FoldersPanel panel;
    KItemListContainer *container = showPanelAt(panel, deepUrl);
    QVERIFY(container);
    auto *model = static_cast<KFileItemModel *>(container->controller()->model());

    QTRY_VERIFY_WITH_TIMEOUT(model->index(deepUrl) >= 0, 10000);
    QTRY_COMPARE(container->controller()->selectionManager()->currentItem(), model->index(deepUrl));
}

QTEST_MAIN(FoldersPanelTest)

#include "folderspaneltest.moc"
