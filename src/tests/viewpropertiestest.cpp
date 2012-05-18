/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "dolphin_generalsettings.h"
#include "views/viewproperties.h"
#include "testdir.h"

#include <QDebug>
#include <QDir>

class ViewPropertiesTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testReadOnlyBehavior();
    void testAutoSave();

private:
    bool m_globalViewProps;
    TestDir* m_testDir;
};

void ViewPropertiesTest::init()
{
    m_globalViewProps = GeneralSettings::self()->globalViewProps();
    GeneralSettings::self()->setGlobalViewProps(false);

    // It is mandatory to create the test-directory inside the home-directory
    // of the user: ViewProperties does not write inside directories
    // outside the home-directory to prevent overwriting other user-settings
    // in case if write-permissions are given.
    m_testDir = new TestDir(QDir::homePath() + "/.viewPropertiesTest-");
}

void ViewPropertiesTest::cleanup()
{
    delete m_testDir;
    m_testDir = 0;

    GeneralSettings::self()->setGlobalViewProps(m_globalViewProps);
}

/**
 * Test whether only reading properties won't result in creating
 * a .directory file when destructing the ViewProperties instance
 * and autosaving is enabled.
 */
void ViewPropertiesTest::testReadOnlyBehavior()
{
    QString dotDirectoryFile = m_testDir->url().toLocalFile() + ".directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));

    ViewProperties* props = new ViewProperties(m_testDir->url());
    QVERIFY(props->isAutoSaveEnabled());
    const QByteArray sortRole = props->sortRole();
    Q_UNUSED(sortRole);
    delete props;
    props = 0;

    QVERIFY(!QFile::exists(dotDirectoryFile));
}

void ViewPropertiesTest::testAutoSave()
{
    QString dotDirectoryFile = m_testDir->url().toLocalFile() + ".directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));

    ViewProperties* props = new ViewProperties(m_testDir->url());
    QVERIFY(props->isAutoSaveEnabled());
    props->setSortRole("someNewSortRole");
    delete props;
    props = 0;

    QVERIFY(QFile::exists(dotDirectoryFile));
}

QTEST_KDEMAIN(ViewPropertiesTest, NoGUI)

#include "viewpropertiestest.moc"
