/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "views/viewproperties.h"
#include "dolphin_generalsettings.h"
#include "testdir.h"

#include <KFileMetaData/UserMetaData>

#include <QStorageInfo>
#include <QTest>

class ViewPropertiesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testReadOnlyBehavior();
    void testReadOnlyDirectory();
    void testAutoSave();
    void testParamMigrationToFileAttr();
    void testParamMigrationToFileAttrKeepDirectory();
    void testExtendedAttributeFull();
    void testUseAsDefaultViewSettings();

private:
    bool m_globalViewProps;
    TestDir *m_testDir;
};

void ViewPropertiesTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    GeneralSettings::self()->setViewPropsTimestamp(QDateTime::currentDateTime());
    QVERIFY(GeneralSettings::self()->save());
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

void ViewPropertiesTest::testReadOnlyDirectory()
{
    const QUrl testDirUrl = m_testDir->url();
    const QString localFolder = testDirUrl.toLocalFile();
    const QString dotDirectoryFile = localFolder + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));

    // restrict write permissions
    QVERIFY(QFile(localFolder).setPermissions(QFileDevice::ReadOwner));

    QScopedPointer<ViewProperties> props(new ViewProperties(testDirUrl));
    const QString destinationDir = props->destinationDir(QStringLiteral("local")) + localFolder;

    QVERIFY(props->isAutoSaveEnabled());
    props->setSortRole("someNewSortRole");
    props.reset();

    qDebug() << destinationDir;
    QVERIFY(QDir(destinationDir).exists());

    QVERIFY(!QFile::exists(dotDirectoryFile));
    KFileMetaData::UserMetaData metadata(localFolder);
    const QString viewProperties = metadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
    QVERIFY(viewProperties.isEmpty());

    props.reset(new ViewProperties(testDirUrl));
    QVERIFY(props->isAutoSaveEnabled());
    QCOMPARE(props->sortRole(), "someNewSortRole");
    props.reset();

    metadata = KFileMetaData::UserMetaData(destinationDir);
    if (metadata.isSupported()) {
        QVERIFY(metadata.hasAttribute("kde.fm.viewproperties#1"));
    } else {
        QVERIFY(QFile::exists(destinationDir + "/.directory"));
    }

    // un-restrict write permissions
    QFile(localFolder).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

void ViewPropertiesTest::testAutoSave()
{
    QString dotDirectoryFile = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));

    QScopedPointer<ViewProperties> props(new ViewProperties(m_testDir->url()));
    QVERIFY(props->isAutoSaveEnabled());
    props->setSortRole("someNewSortRole");
    props.reset();

    KFileMetaData::UserMetaData metadata(m_testDir->url().toLocalFile());
    if (metadata.isSupported()) {
        auto viewProperties = metadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
        QVERIFY(!viewProperties.isEmpty());
        QVERIFY(!QFile::exists(dotDirectoryFile));
    } else {
        QVERIFY(QFile::exists(dotDirectoryFile));
    }
}

void ViewPropertiesTest::testParamMigrationToFileAttr()
{
    QString dotDirectoryFilePath = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFilePath));

    const char *settingsContent = R"SETTINGS("
[Dolphin]
Version=4
ViewMode=1
Timestamp=2023,12,29,10,44,15.793
VisibleRoles=text,CustomizedDetails,Details_text,Details_modificationtime,Details_type

[Settings]
HiddenFilesShown=true)SETTINGS";
    auto dotDirectoryFile = QFile(dotDirectoryFilePath);
    QVERIFY(dotDirectoryFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    QTextStream out(&dotDirectoryFile);
    out << settingsContent;
    dotDirectoryFile.close();

    KFileMetaData::UserMetaData metadata(m_testDir->url().toLocalFile());
    {
        QScopedPointer<ViewProperties> props(new ViewProperties(m_testDir->url()));
        QCOMPARE(props->viewMode(), DolphinView::Mode::DetailsView);
        QVERIFY(props->hiddenFilesShown());
        props->save();

        if (metadata.isSupported()) {
            auto viewProperties = metadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
            QVERIFY(!viewProperties.isEmpty());
            QVERIFY(!QFile::exists(dotDirectoryFilePath));
        } else {
            QVERIFY(QFile::exists(dotDirectoryFilePath));
        }
    }

    ViewProperties props(m_testDir->url());
    QCOMPARE(props.viewMode(), DolphinView::Mode::DetailsView);
    QVERIFY(props.hiddenFilesShown());
}

void ViewPropertiesTest::testParamMigrationToFileAttrKeepDirectory()
{
    QString dotDirectoryFilePath = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFilePath));

    const char *settingsContent = R"SETTINGS("
[Dolphin]
Version=4
ViewMode=1
Timestamp=2023,12,29,10,44,15.793
VisibleRoles=text,CustomizedDetails,Details_text,Details_modificationtime,Details_type

[Settings]
HiddenFilesShown=true

[Other]
ThoseShouldBeKept=true
)SETTINGS";
    auto dotDirectoryFile = QFile(dotDirectoryFilePath);
    QVERIFY(dotDirectoryFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    QTextStream out(&dotDirectoryFile);
    out << settingsContent;
    dotDirectoryFile.close();

    KFileMetaData::UserMetaData metadata(m_testDir->url().toLocalFile());
    {
        QScopedPointer<ViewProperties> props(new ViewProperties(m_testDir->url()));
        QCOMPARE(props->viewMode(), DolphinView::Mode::DetailsView);
        QVERIFY(props->hiddenFilesShown());
        props->save();

        if (metadata.isSupported()) {
            auto viewProperties = metadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
            QVERIFY(!viewProperties.isEmpty());
        }

        QVERIFY(QFile::exists(dotDirectoryFilePath));
        KConfig directorySettings(dotDirectoryFilePath, KConfig::SimpleConfig);
        QCOMPARE(directorySettings.groupList(), {"Other"});
    }

    ViewProperties props(m_testDir->url());
    QVERIFY(props.hiddenFilesShown());
    QCOMPARE(props.viewMode(), DolphinView::Mode::DetailsView);

    QVERIFY(QFile::exists(dotDirectoryFilePath));
}

void ViewPropertiesTest::testExtendedAttributeFull()
{
#ifndef Q_OS_UNIX
    QSKIP("Only unix is supported, for this test");
#endif
    QString dotDirectoryFile = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));

    KFileMetaData::UserMetaData metadata(m_testDir->url().toLocalFile());
    if (!metadata.isSupported()) {
        QSKIP("need extended attribute/filesystem metadata to be usefull");
    }

    QStorageInfo storageInfo(m_testDir->url().toLocalFile());
    auto blockSize = storageInfo.blockSize();

    KFileMetaData::UserMetaData::Error result;
    // write a close to block size theorical maximum size for attributes in Linux for ext4
    // and btrfs (4Kib typically) when ReiserFS/XFS allow XATTR_SIZE_MAX (64Kib)
    result = metadata.setAttribute("data", QString(blockSize - 50, 'a'));
    if (result != KFileMetaData::UserMetaData::NoSpace) {
        QSKIP("File system supports metadata bigger than file system block size");
    }

    // write a close to 4k attribute, maximum size in Linux for ext4
    // so next writing the file metadata fails
    result = metadata.setAttribute("data", QString(blockSize - 60, 'a'));
    QCOMPARE(result, KFileMetaData::UserMetaData::NoError);

    QScopedPointer<ViewProperties> props(new ViewProperties(m_testDir->url()));
    QVERIFY(props->isAutoSaveEnabled());
    props->setSortRole("someNewSortRole");
    props.reset();

    if (metadata.isSupported()) {
        auto viewProperties = metadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
        QVERIFY(viewProperties.isEmpty());
        QVERIFY(QFile::exists(dotDirectoryFile));

        QFile dotDirectory(dotDirectoryFile);
        KConfig viewSettings(dotDirectoryFile, KConfig::SimpleConfig);
        QCOMPARE(viewSettings.groupList(), {"Dolphin"});
        QCOMPARE(viewSettings.group("Dolphin").readEntry("SortRole"), "someNewSortRole");
    } else {
        QVERIFY(QFile::exists(dotDirectoryFile));
    }
}

void ViewPropertiesTest::testUseAsDefaultViewSettings()
{
    // Create new test directory for this test to make sure the settings are defaults
    auto testDir = new TestDir(QDir::homePath() + "/.viewPropertiesTest-");
    auto cleanupTestDir = qScopeGuard([testDir] {
        delete testDir;
    });

    // Create a global viewproperties folder
    QUrl globalPropertiesPath =
        QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append("/view_properties/").append(QStringLiteral("global")));
    QVERIFY(QDir().mkpath(globalPropertiesPath.toLocalFile()));
    auto cleanupGlobalDir = qScopeGuard([globalPropertiesPath] {
        QDir().rmdir(globalPropertiesPath.toLocalFile());
    });
    ViewProperties globalProps(globalPropertiesPath);

    // Check that theres no .directory file and metadata is supported
    QString dotDirectoryFile = testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));
    KFileMetaData::UserMetaData testDirMetadata(testDir->url().toLocalFile());
    KFileMetaData::UserMetaData globalDirMetadata(globalPropertiesPath.toLocalFile());
    if (!testDirMetadata.isSupported()) {
        QSKIP("need extended attribute/filesystem metadata to be usefull");
    }

    const auto newDefaultViewMode = DolphinView::Mode::DetailsView;

    // Equivalent of useAsDefault in ViewPropertiesDialog
    // This sets the DetailsView as default viewMode
    GeneralSettings::setGlobalViewProps(true);
    globalProps.setViewMode(newDefaultViewMode);
    globalProps.setDirProperties(globalProps);
    globalProps.save();
    GeneralSettings::setGlobalViewProps(false);

    // Load default data
    QScopedPointer<ViewProperties> globalDirProperties(new ViewProperties(globalPropertiesPath));
    auto defaultData = globalDirProperties.data();

    // Load testdir data
    QScopedPointer<ViewProperties> testDirProperties(new ViewProperties(testDir->url()));
    auto testData = testDirProperties.data();

    // Make sure globalDirProperties are not empty, so they will be used
    auto globalDirPropString = globalDirMetadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
    QVERIFY(!globalDirPropString.isEmpty());

    // Make sure testDirProperties is empty, so default values are used for it
    auto testDirPropString = testDirMetadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
    QVERIFY(testDirPropString.isEmpty());

    // Compare that default and new folder viewMode is the new default
    QCOMPARE(defaultData->viewMode(), newDefaultViewMode);
    QCOMPARE(testData->viewMode(), defaultData->viewMode());
}

QTEST_GUILESS_MAIN(ViewPropertiesTest)

#include "viewpropertiestest.moc"
