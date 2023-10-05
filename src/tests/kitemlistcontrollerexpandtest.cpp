/*
 * SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "testdir.h"

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class KItemListControllerExpandTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testDirExpand();

private:
    KFileItemListView *m_view;
    KItemListController *m_controller;
    KItemListSelectionManager *m_selectionManager;
    KFileItemModel *m_model;
    TestDir *m_testDir;
    KItemListContainer *m_container;
    QSignalSpy *m_spyDirectoryLoadingCompleted;
};

void KItemListControllerExpandTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    qRegisterMetaType<KItemSet>("KItemSet");

    m_testDir = new TestDir();
    m_model = new KFileItemModel();
    m_view = new KFileItemListView();
    m_controller = new KItemListController(m_model, m_view, this);
    m_container = new KItemListContainer(m_controller);
#ifndef QT_NO_ACCESSIBILITY
    m_view->setAccessibleParentsObject(m_container);
#endif
    m_controller = m_container->controller();
    m_controller->setSelectionBehavior(KItemListController::MultiSelection);
    m_selectionManager = m_controller->selectionManager();

    QStringList files;
    files << "dir1/file1";
    files << "dir1/file2";
    files << "dir1/file3";
    files << "dir1/file4";
    files << "dir1/file5";

    files << "dir2/file1";
    files << "dir2/file2";
    files << "dir2/file3";
    files << "dir2/file4";
    files << "dir2/file5";

    files << "dir3/file1";
    files << "dir3/file2";
    files << "dir3/file3";
    files << "dir3/file4";
    files << "dir3/file5";

    m_testDir->createFiles(files);
    m_model->loadDirectory(m_testDir->url());
    m_spyDirectoryLoadingCompleted = new QSignalSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    QVERIFY(m_spyDirectoryLoadingCompleted->wait());

    m_container->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_container));
}
void KItemListControllerExpandTest::cleanupTestCase()
{
    delete m_container;
    m_container = nullptr;

    delete m_testDir;
    m_testDir = nullptr;
}

void KItemListControllerExpandTest::init()
{
    m_selectionManager->setCurrentItem(0);
    QCOMPARE(m_selectionManager->currentItem(), 0);

    m_selectionManager->clearSelection();
    QVERIFY(!m_selectionManager->hasSelection());
}

void KItemListControllerExpandTest::cleanup()
{
}

void KItemListControllerExpandTest::testDirExpand()
{
    m_view->setItemLayout(KFileItemListView::DetailsLayout);
    QCOMPARE(m_view->itemLayout(), KFileItemListView::DetailsLayout);
    m_view->setSupportsItemExpanding(true);

    // intial state
    QCOMPARE(m_spyDirectoryLoadingCompleted->count(), 1);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 0);
    QCOMPARE(m_selectionManager->selectedItems().count(), 0);

    // extend first folder
    QTest::keyClick(m_container, Qt::Key_Right);
    QVERIFY(m_spyDirectoryLoadingCompleted->wait());
    QCOMPARE(m_model->count(), 8);
    QCOMPARE(m_selectionManager->currentItem(), 0);
    QCOMPARE(m_selectionManager->selectedItems().count(), 0);

    // collapse folder
    QTest::keyClick(m_container, Qt::Key_Left);

    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 0);
    QCOMPARE(m_selectionManager->selectedItems().count(), 0);

    // make the first folder selected
    QTest::keyClick(m_container, Qt::Key_Down);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 1);
    QCOMPARE(m_selectionManager->selectedItems().count(), 1);

    QTest::keyClick(m_container, Qt::Key_Up);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 0);
    QCOMPARE(m_selectionManager->selectedItems().count(), 1);

    // expand the two first folders
    QTest::keyClick(m_container, Qt::Key_Down, Qt::ShiftModifier);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 1);
    QCOMPARE(m_selectionManager->selectedItems().count(), 2);

    // precondition
    QCOMPARE(m_spyDirectoryLoadingCompleted->count(), 2);

    // expand selected folders
    QTest::keyClick(m_container, Qt::Key_Right);
    QVERIFY(QTest::qWaitFor(
        [this]() {
            return m_spyDirectoryLoadingCompleted->count() == 3;
        },
        100));
    QCOMPARE(m_model->count(), 8);
    QCOMPARE(m_selectionManager->currentItem(), 6);
    QCOMPARE(m_selectionManager->selectedItems().count(), 2);

    // collapse the folders
    QTest::keyClick(m_container, Qt::Key_Left);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 1);
    QCOMPARE(m_selectionManager->selectedItems().count(), 2);

    // select third folder
    QTest::keyClick(m_container, Qt::Key_Down, Qt::ShiftModifier);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 2);
    QCOMPARE(m_selectionManager->selectedItems().count(), 3);

    // precondition
    QCOMPARE(m_spyDirectoryLoadingCompleted->count(), 3);

    // expand the three folders
    QTest::keyClick(m_container, Qt::Key_Right);

    QVERIFY(QTest::qWaitFor(
        [this]() {
            return m_spyDirectoryLoadingCompleted->count() == 6;
        },
        300));

    QCOMPARE(m_model->count(), 18);
    QCOMPARE(m_selectionManager->currentItem(), 12);
    QCOMPARE(m_selectionManager->selectedItems().count(), 3);

    // collapse the folders
    QTest::keyClick(m_container, Qt::Key_Left);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 2);
    QCOMPARE(m_selectionManager->selectedItems().count(), 3);

    // shift select the directories
    QTest::keyClick(m_container, Qt::Key_Up);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 1);
    QCOMPARE(m_selectionManager->selectedItems().count(), 1);

    QTest::keyClick(m_container, Qt::Key_Up);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 0);
    QCOMPARE(m_selectionManager->selectedItems().count(), 1);

    QTest::keyClick(m_container, Qt::Key_Down, Qt::ShiftModifier);
    QTest::keyClick(m_container, Qt::Key_Down, Qt::ShiftModifier);

    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 2);
    QCOMPARE(m_selectionManager->selectedItems().count(), 3);

    // expand the three folders with shift modifier
    QTest::keyClick(m_container, Qt::Key_Right, Qt::ShiftModifier);

    QVERIFY(QTest::qWaitFor(
        [this]() {
            return m_spyDirectoryLoadingCompleted->count() == 9;
        },
        100));

    QCOMPARE(m_model->count(), 18);
    QCOMPARE(m_selectionManager->currentItem(), 12);
    QCOMPARE(m_selectionManager->selectedItems().count(), 13);

    // collapse the folders
    QTest::keyClick(m_container, Qt::Key_Left);
    QCOMPARE(m_model->count(), 3);
    QCOMPARE(m_selectionManager->currentItem(), 2);
    QCOMPARE(m_selectionManager->selectedItems().count(), 3);
}

QTEST_MAIN(KItemListControllerExpandTest)

#include "kitemlistcontrollerexpandtest.moc"
