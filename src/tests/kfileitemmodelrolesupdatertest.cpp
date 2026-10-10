/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/kfileitemmodelrolesupdater.h"
#include "kitemviews/kfileitemmodel.h"
#include "testdir.h"

#include <KSambaShare>

#include <QSignalSpy>
#include <QTest>

class KFileItemModelRolesUpdaterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSharesChangedUpdatesOverlays();
};

void KFileItemModelRolesUpdaterTest::testSharesChangedUpdatesOverlays()
{
#ifdef Q_OS_WIN
    QSKIP("KFileItem::overlays() does not look at the samba shares on Windows");
#endif
    TestDir testDir;
    testDir.createDir(QStringLiteral("folder"));

    KFileItemModel model;
    KFileItemModelRolesUpdater updater(&model);
    updater.setRoles({"text", "type"});
    QSignalSpy loadingCompletedSpy(&model, &KFileItemModel::directoryLoadingCompleted);
    model.loadDirectory(testDir.url());
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(model.count(), 1);
    updater.setVisibleIndexRange(0, 1);
    // rolesData() resolved the folder, which connects to KSambaShare.
    QTRY_VERIFY(model.data(0).contains("type"));

    // An overlay that KSambaShare no longer reports, written without the updater seeing it.
    SmallHash data = model.data(0);
    data.insert("iconOverlays", QStringList{QStringLiteral("emblem-shared")});
    model.blockSignals(true);
    model.setData(0, data);
    model.blockSignals(false);

    KSambaShare *const shares = KSambaShare::instance();
    Q_EMIT shares->changed();
    QTRY_VERIFY(!model.data(0).value("iconOverlays").toStringList().contains(QStringLiteral("emblem-shared")));
}

QTEST_MAIN(KFileItemModelRolesUpdaterTest)

#include "kfileitemmodelrolesupdatertest.moc"
