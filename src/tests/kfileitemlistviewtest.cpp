/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include <KDirLister>
#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "testdir.h"

#include <QGraphicsView>

namespace {
    const int DefaultTimeout = 2000;
};

class KFileItemListViewTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

private:
    KFileItemListView* m_listView;
    KFileItemModel* m_model;
    KDirLister* m_dirLister;
    TestDir* m_testDir;
    QGraphicsView* m_graphicsView;
};

void KFileItemListViewTest::init()
{
    qRegisterMetaType<KItemRangeList>("KItemRangeList");
    qRegisterMetaType<KFileItemList>("KFileItemList");

    m_testDir = new TestDir();
    m_dirLister = new KDirLister();
    m_model = new KFileItemModel(m_dirLister);

    m_listView = new KFileItemListView();
    m_listView->onModelChanged(m_model, 0);

    m_graphicsView = new QGraphicsView();
    m_graphicsView->show();
    QTest::qWaitForWindowShown(m_graphicsView);
}

void KFileItemListViewTest::cleanup()
{
    delete m_graphicsView;
    m_graphicsView = 0;

    delete m_listView;
    m_listView = 0;

    delete m_model;
    m_model = 0;

    delete m_dirLister;
    m_dirLister = 0;

    delete m_testDir;
    m_testDir = 0;
}

QTEST_KDEMAIN(KFileItemListViewTest, GUI)

#include "kfileitemlistviewtest.moc"
