/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphin_generalsettings.h"
#include "views/viewproperties.h"
#include "testdir.h"

#include <QTest>

class ViewPropertiesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testReadOnlyBehavior();
    void testAutoSave();

private:
    bool m_globalViewProps;
    TestDir* m_testDir;
};

void ViewPropertiesTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void ViewPropertiesTest::init()
{
    m_globalViewProps = GeneralSettings::self()->globalViewProps();
    GeneralSettings::self()->setGlobalViewProps(false);
    GeneralSettings::self()->save();

    // It is mandatory to create the test-directory inside the home-directory
    // of the user: ViewProperties does not write inside directories
    // outside the home-directory to prevent overwriting other user-settings
    // in case if write-permissions are given.
    m_testDir = new TestDir(QDir::homePath() + "/.viewPropertiesTest-");
}

void ViewPropertiesTest::cleanup()
{
    delete m_testDir;
    m_testDir = nullptr;

    GeneralSettings::self()->setGlobalViewProps(m_globalViewProps);
    GeneralSettings::self()->save();
}

/**
 * Test whether only reading properties won't result in creating
 * a .directory file when destructing the ViewProperties instance
 * and autosaving is enabled.
 */
void ViewPropertiesTest::testReadOnlyBehavior()
{
    QString dotDirectoryFile = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));

    QScopedPointer<ViewProperties> props(new ViewProperties(m_testDir->url()));
    QVERIFY(props->isAutoSaveEnabled());
    const QByteArray sortRole = props->sortRole();
    Q_UNUSED(sortRole)
    props.reset();

    QVERIFY(!QFile::exists(dotDirectoryFile));
}

void ViewPropertiesTest::testAutoSave()
{
    QString dotDirectoryFile = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));

    QScopedPointer<ViewProperties> props(new ViewProperties(m_testDir->url()));
    QVERIFY(props->isAutoSaveEnabled());
    props->setSortRole("someNewSortRole");
    props.reset();

    QVERIFY(QFile::exists(dotDirectoryFile));
}

QTEST_GUILESS_MAIN(ViewPropertiesTest)

#include "viewpropertiestest.moc"
