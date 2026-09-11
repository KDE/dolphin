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
#include <QTemporaryDir>
#include <QTest>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

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
    void testGlobalDefaultConfigFromDirectory();
    void testExtendedAttributeFull();
    void testExtendedAttributeFullKeepDirectory();
    void testExtendedAttributeFullWriteFailure();
    void testUseAsDefaultViewSettings();
    void testUseAsCustomDefaultViewSettings();
    void testSpecialFolderPropsPreservedWithGlobalViewProps();
    void testDownloadsKeepsTheOrdinaryStyleWhenChosen();
    void testDownloadsHasItsOwnStyleWithGlobalViewProps();
    void testAnEmptyUrlHoldsTheStyleEveryFolderStartsFrom();
    void testWhichFoldersComeWithAStyleOfTheirOwn();
    void testAStoreThatWillNotOpenReadsTheStyleTheFolderComesWith();
    void testChangingAFolderWithItsOwnStyleOffersTheRestore();
    void testRestoringWithoutAutoSaveWaitsForTheSave();
    void testARemovalThatFailsLeavesTheStyleWrittenDown();
    void testVisitingAFolderWithItsOwnStyleWritesNothingDown();
    void testRestoringASpecialFolderGivesBackItsOwnStyle();
    void testRestoringTheDownloadsFolderGivesBackItsOwnStyle();
    void testRestoreViewProps();
    void testRemotePropsPerFolder();
    void testLocalFallbackMigration();
    void testSymlinkSharesProperties();

private:
    bool m_globalViewProps;
    TestDir *m_testDir;
    // These tests write to the downloads folder, which is read from the home directory, so they are
    // given a home directory of their own rather than the one of whoever runs them. It sits in that
    // one, so that the tests keep the file system they would have run on.
    QTemporaryDir m_home{QDir::homePath() + QStringLiteral("/.viewPropertiesHome-")};
    QString m_downloadsPath;
};

void ViewPropertiesTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    QVERIFY(m_home.isValid());
    qputenv("HOME", QFile::encodeName(m_home.path()));

    // Where the downloads folder is read from differs by platform, so it is only used where moving
    // the home directory moved it as well.
    const QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadsPath.startsWith(m_home.path() + QDir::separator())) {
        QVERIFY(QDir().mkpath(downloadsPath));
        m_downloadsPath = downloadsPath;
    }

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

    // Check that we do not have temp files left after test case.
    QDir tempDir(QDir::tempPath());
    QVERIFY(tempDir.entryList(QStringList() << QCoreApplication::applicationName() + "*", QDir::Files).length() == 0);
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
    const QString destinationDir = props->destinationDir(QStringLiteral("local/")) + ViewProperties::directoryHashForUrl(testDirUrl);

    QVERIFY(props->isAutoSaveEnabled());
    props->setSortRole("someNewSortRole");
    props.reset();

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

void ViewPropertiesTest::testGlobalDefaultConfigFromDirectory()
{
    QUrl globalPropertiesPath =
        QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append("/view_properties/").append(QStringLiteral("global")));
    QVERIFY(QDir().mkpath(globalPropertiesPath.toLocalFile()));

    QString dotDirectoryFilePath = globalPropertiesPath.toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFilePath));

    auto cleanupGlobalDir = qScopeGuard([globalPropertiesPath, dotDirectoryFilePath] {
        QFile::remove(dotDirectoryFilePath);
        QDir().rmdir(globalPropertiesPath.toLocalFile());
    });

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

    ViewProperties props(m_testDir->url());
    props.save();

    // We delete a temporary file in 'ViewProperties::save()' after reading the default config,
    // and that temp file is created as copy from '.directory' if we have metadata enabled.
    //
    // But it can be original '.directory' instead of temp file,
    // if we read default config from 'global' directory, which does not support attributes.
    // So we make sure that it is not deleted here,
    // because we do not want to delete '.directory' file.
    QVERIFY(QFile::exists(dotDirectoryFilePath));

    QVERIFY(props.hiddenFilesShown());
    QCOMPARE(props.viewMode(), DolphinView::Mode::DetailsView);
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
        QSKIP("need extended attribute/filesystem metadata to be useful");
    }

    QStorageInfo storageInfo(m_testDir->url().toLocalFile());
    auto blockSize = storageInfo.blockSize();

    KFileMetaData::UserMetaData::Error result;
    // write a close to block size theoretical maximum size for attributes in Linux for ext4
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

void ViewPropertiesTest::testExtendedAttributeFullKeepDirectory()
{
#ifndef Q_OS_UNIX
    QSKIP("Only unix is supported, for this test");
#endif
    const QString dotDirectoryFilePath = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFilePath));

    // Pre-populate .directory with a Dolphin group and a non-Dolphin group (e.g. folder icon).
    const char *initialContent =
        "[Desktop Entry]\n"
        "Icon=folder-pictures\n"
        "\n"
        "[Dolphin]\n"
        "Version=4\n"
        "ViewMode=0\n";
    {
        QFile f(dotDirectoryFilePath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
        f.write(initialContent);
    }

    KFileMetaData::UserMetaData metadata(m_testDir->url().toLocalFile());
    if (!metadata.isSupported()) {
        QSKIP("need extended attribute/filesystem metadata to be useful");
    }

    QStorageInfo storageInfo(m_testDir->url().toLocalFile());
    const auto blockSize = storageInfo.blockSize();

    KFileMetaData::UserMetaData::Error result;
    result = metadata.setAttribute("data", QString(blockSize - 50, 'a'));
    if (result != KFileMetaData::UserMetaData::NoSpace) {
        QSKIP("File system supports metadata bigger than file system block size");
    }
    result = metadata.setAttribute("data", QString(blockSize - 60, 'a'));
    QCOMPARE(result, KFileMetaData::UserMetaData::NoError);

    QScopedPointer<ViewProperties> props(new ViewProperties(m_testDir->url()));
    props->setSortRole("someNewSortRole");
    props.reset();

    // xattr write should have failed with NoSpace — attribute must be empty
    QVERIFY(metadata.attribute(QStringLiteral("kde.fm.viewproperties#1")).isEmpty());
    // .directory must still exist
    QVERIFY(QFile::exists(dotDirectoryFilePath));

    KConfig viewSettings(dotDirectoryFilePath, KConfig::SimpleConfig);
    // Dolphin settings were updated
    QCOMPARE(viewSettings.group(QStringLiteral("Dolphin")).readEntry("SortRole"), QStringLiteral("someNewSortRole"));
    // Non-Dolphin groups must be preserved
    QVERIFY(viewSettings.hasGroup(QStringLiteral("Desktop Entry")));
    QCOMPARE(viewSettings.group(QStringLiteral("Desktop Entry")).readEntry("Icon"), QStringLiteral("folder-pictures"));
}

void ViewPropertiesTest::testExtendedAttributeFullWriteFailure()
{
#ifndef Q_OS_UNIX
    QSKIP("Only unix is supported, for this test");
#else
    if (getuid() == 0) {
        QSKIP("Running as root — permission checks are not enforced");
    }

    const QString testDirPath = m_testDir->url().toLocalFile();
    const QString dotDirectoryFilePath = testDirPath + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFilePath));

    KFileMetaData::UserMetaData metadata(testDirPath);
    if (!metadata.isSupported()) {
        QSKIP("need extended attribute/filesystem metadata to be useful");
    }

    QStorageInfo storageInfo(testDirPath);
    const auto blockSize = storageInfo.blockSize();

    KFileMetaData::UserMetaData::Error result;
    result = metadata.setAttribute("data", QString(blockSize - 50, 'a'));
    if (result != KFileMetaData::UserMetaData::NoSpace) {
        QSKIP("File system supports metadata bigger than file system block size");
    }
    result = metadata.setAttribute("data", QString(blockSize - 60, 'a'));
    QCOMPARE(result, KFileMetaData::UserMetaData::NoError);

    // Make the test directory read-only so writing .directory fails
    QVERIFY(QFile::setPermissions(testDirPath, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    auto restorePermissions = qScopeGuard([&testDirPath] {
        QFile::setPermissions(testDirPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    });

    // This must not crash even though both xattr and .directory are unwritable
    QScopedPointer<ViewProperties> props(new ViewProperties(m_testDir->url()));
    props->setSortRole("someNewSortRole");
    props.reset();

    // Neither xattr nor .directory should have been written
    QVERIFY(metadata.attribute(QStringLiteral("kde.fm.viewproperties#1")).isEmpty());
    QVERIFY(!QFile::exists(dotDirectoryFilePath));
#endif
}

void ViewPropertiesTest::testUseAsDefaultViewSettings()
{
    // Create a global viewproperties folder
    QUrl globalPropertiesPath =
        QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append("/view_properties/").append(QStringLiteral("global")));
    QVERIFY(QDir().mkpath(globalPropertiesPath.toLocalFile()));
    auto cleanupGlobalDir = qScopeGuard([globalPropertiesPath] {
        QDir().rmdir(globalPropertiesPath.toLocalFile());
    });
    ViewProperties globalProps(globalPropertiesPath);

    // Check that there's no .directory file and metadata is supported
    QString dotDirectoryFile = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));
    KFileMetaData::UserMetaData testDirMetadata(m_testDir->url().toLocalFile());
    KFileMetaData::UserMetaData globalDirMetadata(globalPropertiesPath.toLocalFile());
    if (!testDirMetadata.isSupported()) {
        QSKIP("need extended attribute/filesystem metadata to be useful");
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
    QScopedPointer<ViewProperties> testDirProperties(new ViewProperties(m_testDir->url()));
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

void ViewPropertiesTest::testUseAsCustomDefaultViewSettings()
{
    // Create a global viewproperties folder
    QUrl globalPropertiesPath =
        QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).append("/view_properties/").append(QStringLiteral("global")));
    QVERIFY(QDir().mkpath(globalPropertiesPath.toLocalFile()));
    auto cleanupGlobalDir = qScopeGuard([globalPropertiesPath] {
        QDir().rmdir(globalPropertiesPath.toLocalFile());
    });
    ViewProperties globalProps(globalPropertiesPath);

    // Check that there's no .directory file and metadata is supported
    QString dotDirectoryFile = m_testDir->url().toLocalFile() + "/.directory";
    QVERIFY(!QFile::exists(dotDirectoryFile));
    KFileMetaData::UserMetaData testDirMetadata(m_testDir->url().toLocalFile());
    KFileMetaData::UserMetaData globalDirMetadata(globalPropertiesPath.toLocalFile());
    if (!testDirMetadata.isSupported()) {
        QSKIP("need extended attribute/filesystem metadata to be useful");
    }

    // Equivalent of useAsDefault in ViewPropertiesDialog
    // This sets the DetailsView as default viewMode
    GeneralSettings::setGlobalViewProps(true);
    globalProps.setViewMode(DolphinView::Mode::DetailsView);
    globalProps.setDirProperties(globalProps);
    globalProps.save();
    GeneralSettings::setGlobalViewProps(false);

    // Make sure globalDirProperties are not empty, so they will be used
    auto globalDirPropString = globalDirMetadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
    QVERIFY(!globalDirPropString.isEmpty());

    // Load default data
    QScopedPointer<ViewProperties> globalDirProperties(new ViewProperties(globalPropertiesPath));
    auto defaultData = globalDirProperties.data();
    QCOMPARE(defaultData->viewMode(), DolphinView::Mode::DetailsView);

    // Load testdir data, set to icon, i.e default hardcoded, not current user default
    QScopedPointer<ViewProperties> testDirProperties(new ViewProperties(m_testDir->url()));
    testDirProperties->setViewMode(DolphinView::Mode::IconsView);
    testDirProperties->save();

    // testDirProperties is not default
    auto testDirPropString = testDirMetadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
    QVERIFY(!testDirPropString.isEmpty());
    QCOMPARE(testDirProperties.data()->viewMode(), DolphinView::Mode::IconsView);

    // testDirProperties is now default
    testDirProperties->setViewMode(DolphinView::Mode::DetailsView);
    testDirProperties->save();

    // no more metedata => the defaults settings are in effect for the folder
    testDirPropString = testDirMetadata.attribute(QStringLiteral("kde.fm.viewproperties#1"));
    QVERIFY(testDirPropString.isEmpty());
    QCOMPARE(testDirProperties.data()->viewMode(), DolphinView::Mode::DetailsView);
}

/**
 * Test that special folders preserve user-customized view properties
 * when global view props is enabled. Uses trash:/// as representative example.
 */
void ViewPropertiesTest::testSpecialFolderPropsPreservedWithGlobalViewProps()
{
    GeneralSettings::self()->setGlobalViewProps(true);
    GeneralSettings::self()->save();

    const QUrl trashUrl(QStringLiteral("trash:///"));

    // First visit: customize the view mode to IconsView
    // (trash default is DetailsView)
    {
        ViewProperties props(trashUrl);
        QCOMPARE(props.viewMode(), DolphinView::DetailsView); // default
        props.setViewMode(DolphinView::IconsView);
        props.save();
    }

    // Second visit: the custom view mode should be preserved
    {
        ViewProperties props(trashUrl);
        QCOMPARE(props.viewMode(), DolphinView::IconsView);
    }
}

void ViewPropertiesTest::testDownloadsKeepsTheOrdinaryStyleWhenChosen()
{
    // init() has already turned per-folder view properties on.
    if (m_downloadsPath.isEmpty()) {
        QSKIP("The downloads folder is not inside the home directory these tests were given");
    }
    const QString downloadsPath = m_downloadsPath;
    const QUrl downloadsUrl = QUrl::fromLocalFile(downloadsPath);

    auto forgetStoredProperties = [downloadsPath]() {
        KFileMetaData::UserMetaData metadata(downloadsPath);
        if (metadata.isSupported() && metadata.hasAttribute(QStringLiteral("kde.fm.viewproperties#1"))) {
            metadata.setAttribute(QStringLiteral("kde.fm.viewproperties#1"), QString());
        }
        QFile::remove(downloadsPath + QDir::separator() + QStringLiteral(".directory"));
    };
    // Each test starts from a folder with nothing written down for it.
    forgetStoredProperties();
    auto cleanup = qScopeGuard(forgetStoredProperties);

    // The style every other folder has, written down here so that what an earlier test left behind
    // does not decide the answer.
    {
        ViewProperties globalProps{QUrl()};
        globalProps.setSortRole(QByteArrayLiteral("text"));
        globalProps.setSortOrder(Qt::AscendingOrder);
        globalProps.setSortFoldersFirst(true);
        globalProps.setGroupedSorting(false);
        globalProps.save();
    }

    {
        ViewProperties props(downloadsUrl);
        // the style the downloads folder comes with
        QCOMPARE(props.sortRole(), QByteArrayLiteral("modificationtime"));
        QCOMPARE(props.sortFoldersFirst(), false);
        QCOMPARE(props.groupedSorting(), true);

        // The user asks for the style every other folder has.
        props.setSortRole(QByteArrayLiteral("text"));
        props.setSortOrder(Qt::AscendingOrder);
        props.setSortFoldersFirst(true);
        props.setGroupedSorting(false);
        props.save();
    }

    ViewProperties props(downloadsUrl);
    QCOMPARE(props.sortRole(), QByteArrayLiteral("text"));
    QCOMPARE(props.sortOrder(), Qt::AscendingOrder);
    QCOMPARE(props.sortFoldersFirst(), true);
    QCOMPARE(props.groupedSorting(), false);
}

/**
 * A folder with a display style of its own has it whichever way the user keeps display styles, one
 * for all folders or one per folder.
 */
void ViewPropertiesTest::testDownloadsHasItsOwnStyleWithGlobalViewProps()
{
    if (m_downloadsPath.isEmpty()) {
        QSKIP("The downloads folder is not inside the home directory these tests were given");
    }
    const QString downloadsPath = m_downloadsPath;
    const QUrl downloadsUrl = QUrl::fromLocalFile(downloadsPath);

    auto forgetStoredProperties = [downloadsPath]() {
        KFileMetaData::UserMetaData metadata(downloadsPath);
        if (metadata.isSupported() && metadata.hasAttribute(QStringLiteral("kde.fm.viewproperties#1"))) {
            metadata.setAttribute(QStringLiteral("kde.fm.viewproperties#1"), QString());
        }
        QFile::remove(downloadsPath + QDir::separator() + QStringLiteral(".directory"));
    };
    // Each test starts from a folder with nothing written down for it.
    forgetStoredProperties();
    auto cleanup = qScopeGuard(forgetStoredProperties);

    GeneralSettings::self()->setGlobalViewProps(true);
    GeneralSettings::self()->save();

    ViewProperties props(downloadsUrl);
    QCOMPARE(props.sortRole(), QByteArrayLiteral("modificationtime"));
    QCOMPARE(props.sortOrder(), Qt::DescendingOrder);
    QCOMPARE(props.sortFoldersFirst(), false);
    QCOMPARE(props.groupedSorting(), true);
}

/**
 * Restoring a folder to its defaults means the style every other folder has, except for a folder
 * that comes with one of its own: there it means that one. Checked on the trash and on the images
 * search, which are two of the folders that have one.
 */
void ViewPropertiesTest::testRestoringASpecialFolderGivesBackItsOwnStyle()
{
    // init() has already turned per-folder view properties on.
    const QList<QUrl> specialUrls{QUrl(QStringLiteral("trash:///")), QUrl(QStringLiteral("search:/images"))};

    for (const QUrl &url : specialUrls) {
        ViewProperties(url).forgetStoredProperties();
        auto cleanup = qScopeGuard([url] {
            ViewProperties(url).forgetStoredProperties();
        });

        DolphinView::Mode ownMode;
        {
            ViewProperties props(url);
            ownMode = props.viewMode(); // the style this folder comes with
            QVERIFY2(props.isDefaults(), qPrintable(url.toString()));

            props.setViewMode(ownMode == DolphinView::IconsView ? DolphinView::CompactView : DolphinView::IconsView);
            props.save();
        }

        {
            ViewProperties props(url);
            QVERIFY2(props.viewMode() != ownMode, qPrintable(url.toString()));
            // A choice is written down, so restoring it is worth offering.
            QVERIFY2(!props.isDefaults(), qPrintable(url.toString()));
            props.restoreToDefaults();
            // Restoring leaves the properties in hand showing that style, with no second read.
            QCOMPARE(props.viewMode(), ownMode);
            QVERIFY2(props.isDefaults(), qPrintable(url.toString()));
        }

        ViewProperties props(url);
        QCOMPARE(props.viewMode(), ownMode);
        QVERIFY2(props.isDefaults(), qPrintable(url.toString()));
    }
}

void ViewPropertiesTest::testRestoringTheDownloadsFolderGivesBackItsOwnStyle()
{
    if (m_downloadsPath.isEmpty()) {
        QSKIP("The downloads folder is not inside the home directory these tests were given");
    }
    const QString downloadsPath = m_downloadsPath;
    const QUrl downloadsUrl = QUrl::fromLocalFile(downloadsPath);
    // Each test starts from a folder with nothing written down for it.
    ViewProperties(downloadsUrl).forgetStoredProperties();
    auto cleanup = qScopeGuard([downloadsUrl] {
        ViewProperties(downloadsUrl).forgetStoredProperties();
    });

    {
        ViewProperties props(downloadsUrl);
        QCOMPARE(props.sortRole(), QByteArrayLiteral("modificationtime"));

        // The style every other folder has, which is a choice like any other.
        props.setSortRole(QByteArrayLiteral("text"));
        props.setSortFoldersFirst(true);
        props.setGroupedSorting(false);
        props.save();
    }

    {
        ViewProperties props(downloadsUrl);
        QCOMPARE(props.sortRole(), QByteArrayLiteral("text"));
        QVERIFY(!props.isDefaults());
        props.restoreToDefaults();
        QCOMPARE(props.sortRole(), QByteArrayLiteral("modificationtime"));
        QCOMPARE(props.groupedSorting(), true);
        QVERIFY(props.isDefaults());
    }

    ViewProperties props(downloadsUrl);
    QCOMPARE(props.sortRole(), QByteArrayLiteral("modificationtime"));
    QCOMPARE(props.groupedSorting(), true);
    QVERIFY(props.isDefaults());
}

void ViewPropertiesTest::testRestoreViewProps()
{
    const QUrl testDirUrl = m_testDir->url();

    // First visit: customize the view mode to DetailsView
    {
        ViewProperties props(testDirUrl);
        QCOMPARE(props.viewMode(), DolphinView::IconsView); // default
        props.setViewMode(DolphinView::DetailsView);
    }
    {
        ViewProperties props(testDirUrl);
        QCOMPARE(props.viewMode(), DolphinView::DetailsView);
        QVERIFY(!props.isDefaults());
    }
    {
        ViewProperties props(testDirUrl);
        props.restoreToDefaults();
        QVERIFY(props.isDefaults());
        QCOMPARE(props.viewMode(), DolphinView::IconsView);
    }
}

/**
 * Regression test for https://bugs.kde.org/show_bug.cgi?id=220326: each
 * non-local (remote) folder must keep its own view properties rather than all
 * of them sharing a single "remote" configuration.
 */
void ViewPropertiesTest::testRemotePropsPerFolder()
{
    // Start from a clean remote store so a previous run does not influence us.
    const QString remoteBase = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/view_properties/remote");
    QDir(remoteBase).removeRecursively();

    const QUrl folderA(QStringLiteral("smb://server/share/folderA"));
    const QUrl folderB(QStringLiteral("smb://server/share/folderB"));

    // Read the default view mode, then pick a different one to apply.
    DolphinView::Mode defaultMode;
    {
        ViewProperties props(folderA);
        defaultMode = props.viewMode();
    }
    const DolphinView::Mode customMode = defaultMode == DolphinView::IconsView ? DolphinView::DetailsView : DolphinView::IconsView;

    // Customize and save the first remote folder.
    {
        ViewProperties props(folderA);
        props.setViewMode(customMode);
        props.save();
    }

    // The first remote folder remembers its customization across instances.
    {
        ViewProperties props(folderA);
        QCOMPARE(props.viewMode(), customMode);
    }

    // A different remote folder keeps the default, not the first folder's mode.
    {
        ViewProperties props(folderB);
        QCOMPARE(props.viewMode(), defaultMode);
    }
}

/**
 * The local fallback store (used when a folder's own properties cannot be
 * written) used to key entries by the full local path. They are now keyed by a
 * hash; check that an entry written the old way is migrated to the new location
 * on first access and its properties are preserved.
 */
void ViewPropertiesTest::testLocalFallbackMigration()
{
    const QUrl testDirUrl = m_testDir->url();
    const QString localFolder = testDirUrl.toLocalFile();

    // Make the folder read-only so the fallback store is used rather than the
    // folder itself.
    QVERIFY(QFile(localFolder).setPermissions(QFileDevice::ReadOwner));
    auto restorePermissions = qScopeGuard([&localFolder] {
        QFile(localFolder).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    });

    const QString localBase = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/view_properties/local");
#ifdef Q_OS_WIN
    // On Windows a colon is only allowed right after the drive letter, so the stored
    // path drops the drive colon and sits after a separator.
    const QString oldPath = localBase + QDir::separator() + QString(localFolder).remove(QLatin1Char(':'));
#else
    const QString oldPath = localBase + localFolder;
#endif
    const QString newPath = localBase + QStringLiteral("/") + ViewProperties::directoryHashForUrl(testDirUrl);

    // Write an entry the way an older Dolphin would have, under the full path.
    QVERIFY(QDir().mkpath(oldPath));
    {
        KConfig oldConfig(oldPath + QStringLiteral("/.directory"), KConfig::SimpleConfig);
        oldConfig.group(QStringLiteral("Dolphin")).writeEntry("ViewMode", int(DolphinView::DetailsView));
        oldConfig.sync();
    }
    QVERIFY(!QFileInfo::exists(newPath));

    // Constructing ViewProperties migrates the entry to the hashed location and
    // reads the preserved view mode (the default would be IconsView).
    {
        ViewProperties props(testDirUrl);
        QCOMPARE(props.viewMode(), DolphinView::DetailsView);
    }

    QVERIFY(QFileInfo::exists(newPath));
    QVERIFY(!QFileInfo::exists(oldPath));
}

/**
 * A folder and a symbolic link pointing at it reference the same content, so
 * they should share their view properties. When the properties are kept in the
 * local fallback store (here forced by making the folder read-only), they used
 * to be keyed by the requested path, so the link and its target got separate
 * entries. Check that they now resolve to the same entry. See bug 477662.
 */
void ViewPropertiesTest::testSymlinkSharesProperties()
{
#ifndef Q_OS_UNIX
    // QFile::link creates a shortcut file on Windows rather than a symbolic link that
    // canonicalFilePath resolves, so the shared-properties behaviour cannot be exercised.
    QSKIP("Only unix is supported, for this test");
#endif
    const QUrl targetUrl = m_testDir->url();
    const QString targetFolder = targetUrl.toLocalFile();

    const QString linkPath = QDir::homePath() + QStringLiteral("/.viewPropertiesTestLink-") + QString::number(QCoreApplication::applicationPid());
    QVERIFY(QFile::link(targetFolder, linkPath));
    auto removeLink = qScopeGuard([&linkPath] {
        QFile::remove(linkPath);
    });

    // Make the folder read-only so the local fallback store is used rather than
    // the folder itself.
    QVERIFY(QFile(targetFolder).setPermissions(QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    auto restorePermissions = qScopeGuard([&targetFolder] {
        QFile(targetFolder).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    });

    // Customize the view mode through the real folder.
    {
        ViewProperties props(targetUrl);
        props.setViewMode(DolphinView::CompactView);
    }

    // Accessing the folder through the symlink yields the same properties; the
    // default would be IconsView.
    {
        ViewProperties props(QUrl::fromLocalFile(linkPath));
        QCOMPARE(props.viewMode(), DolphinView::CompactView);
    }
}

// The view properties dialog and the settings page write the style every folder starts from through
// an empty url, so that they reach it wherever the view is, including a folder with a style of its
// own.
void ViewPropertiesTest::testAnEmptyUrlHoldsTheStyleEveryFolderStartsFrom()
{
    // init() has turned per-folder view properties on, and an empty url reaches that style anyway.
    QVERIFY(!GeneralSettings::self()->globalViewProps());

    DolphinView::Mode chosen;
    DolphinView::Mode previous;
    {
        ViewProperties defaults{QUrl()};
        QCOMPARE(defaults.m_filePath, defaults.destinationDir(QStringLiteral("global")));

        previous = defaults.viewMode();
        chosen = previous == DolphinView::CompactView ? DolphinView::IconsView : DolphinView::CompactView;
        defaults.setViewMode(chosen);
        defaults.save();
    }
    // The style every folder starts from is what the rest of the suite reads, so it is put back.
    auto cleanup = qScopeGuard([previous] {
        ViewProperties defaults{QUrl()};
        defaults.setViewMode(previous);
        defaults.save();
    });

    GeneralSettings::self()->setGlobalViewProps(true);
    GeneralSettings::self()->save();
    {
        ViewProperties ordinary(QUrl::fromLocalFile(m_testDir->path()));
        QCOMPARE(ordinary.viewMode(), chosen);
    }
    GeneralSettings::self()->setGlobalViewProps(false);
    GeneralSettings::self()->save();
}

// The settings page asks a folder this before it lets its style become the one every folder starts
// from, so that the trash does not hand every folder the column for the time a file was deleted.
void ViewPropertiesTest::testWhichFoldersComeWithAStyleOfTheirOwn()
{
    QVERIFY(ViewProperties(QUrl(QStringLiteral("trash:///"))).hasSpecialDefaultViewSettings());
    QVERIFY(ViewProperties(QUrl(QStringLiteral("search:/images"))).hasSpecialDefaultViewSettings());
    QVERIFY(ViewProperties(QUrl(QStringLiteral("recentlyused:/files"))).hasSpecialDefaultViewSettings());
    QVERIFY(!ViewProperties(QUrl::fromLocalFile(m_testDir->path())).hasSpecialDefaultViewSettings());

    if (!m_downloadsPath.isEmpty()) {
        QVERIFY(ViewProperties(QUrl::fromLocalFile(m_downloadsPath)).hasSpecialDefaultViewSettings());
    }
}

// Reading a folder that has a style of its own is not a choice, so it leaves nothing written down
// and the entry to restore it stays unoffered.
void ViewPropertiesTest::testVisitingAFolderWithItsOwnStyleWritesNothingDown()
{
    const QList<QUrl> ownStyleUrls{QUrl(QStringLiteral("trash:///")), QUrl(QStringLiteral("search:/images"))};

    for (const QUrl &url : ownStyleUrls) {
        ViewProperties(url).forgetStoredProperties();
        auto cleanup = qScopeGuard([url] {
            ViewProperties(url).forgetStoredProperties();
        });

        DolphinView::Mode ownMode;
        {
            ViewProperties props(url);
            ownMode = props.viewMode();
            QVERIFY2(props.isDefaults(), qPrintable(url.toString()));
        }

        // The visit above is over, and it has recorded nothing of its own.
        ViewProperties props(url);
        QVERIFY2(!props.m_hasStoredProperties, qPrintable(url.toString()));
        QVERIFY2(props.isDefaults(), qPrintable(url.toString()));
        QCOMPARE(props.viewMode(), ownMode);
    }
}

// Whether a style is written down is asked of the storage, and reading it can still fail: a
// .directory that will not open answers with nothing while the extended attribute beside it says
// there is something. The folder is then read with the style it comes with.
void ViewPropertiesTest::testAStoreThatWillNotOpenReadsTheStyleTheFolderComesWith()
{
#ifndef Q_OS_UNIX
    QSKIP("Only unix is supported, for this test");
#else
    if (getuid() == 0) {
        QSKIP("Running as root - permission checks are not enforced");
    }

    const QUrl trashUrl(QStringLiteral("trash:///"));
    const QString storePath = ViewProperties(trashUrl).m_filePath;
    QVERIFY(QDir().mkpath(storePath));

    // A style written down in the extended attribute.
    {
        ViewProperties props(trashUrl);
        props.setViewMode(DolphinView::CompactView);
        props.save();
    }
    const KFileMetaData::UserMetaData metaData(storePath);
    if (!metaData.isSupported() || !metaData.hasAttribute(QStringLiteral("kde.fm.viewproperties#1"))) {
        QSKIP("need extended attribute/filesystem metadata to be useful");
    }

    // And a .directory beside it that cannot be read.
    const QString dotDirectory = storePath + QDir::separator() + QStringLiteral(".directory");
    QFile file(dotDirectory);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[Dolphin]\nViewMode=1\n");
    file.close();
    QVERIFY(QFile::setPermissions(dotDirectory, QFileDevice::Permissions()));
    auto cleanup = qScopeGuard([dotDirectory, trashUrl] {
        QFile::setPermissions(dotDirectory, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QFile::remove(dotDirectory);
        ViewProperties(trashUrl).forgetStoredProperties();
    });

    ViewProperties props(trashUrl);
    QCOMPARE(props.viewMode(), DolphinView::DetailsView);
#endif
}

// Restoring is offered while the view differs from the style the folder comes with, so a change
// made to the properties in hand counts, and not only one that is already written down.
void ViewPropertiesTest::testChangingAFolderWithItsOwnStyleOffersTheRestore()
{
    const QUrl trashUrl(QStringLiteral("trash:///"));
    ViewProperties(trashUrl).forgetStoredProperties();
    auto cleanup = qScopeGuard([trashUrl] {
        ViewProperties(trashUrl).forgetStoredProperties();
    });

    ViewProperties props(trashUrl);
    QVERIFY(props.isDefaults());

    // DolphinView asks this of the properties it just changed, before anything is written down.
    props.setSortRole(QByteArrayLiteral("size"));
    QVERIFY(!props.isDefaults());

    props.save();
    QVERIFY(!props.isDefaults());
}

// A dialog turns auto saving off and writes when the user accepts it, so restoring a folder to the
// style it comes with waits for that write rather than removing what is written down at once.
void ViewPropertiesTest::testRestoringWithoutAutoSaveWaitsForTheSave()
{
    const QUrl trashUrl(QStringLiteral("trash:///"));
    ViewProperties(trashUrl).forgetStoredProperties();
    auto cleanup = qScopeGuard([trashUrl] {
        ViewProperties(trashUrl).forgetStoredProperties();
    });

    auto writeAStyle = [trashUrl] {
        ViewProperties props(trashUrl);
        props.setViewMode(DolphinView::CompactView);
        props.save();
    };

    writeAStyle();
    QVERIFY(ViewProperties(trashUrl).m_hasStoredProperties);

    {
        ViewProperties props(trashUrl);
        props.setAutoSaveEnabled(false);
        props.restoreToDefaults();
    }
    QVERIFY2(ViewProperties(trashUrl).m_hasStoredProperties, "a restore that was never written removed the style anyway");

    {
        ViewProperties props(trashUrl);
        props.setAutoSaveEnabled(false);
        props.restoreToDefaults();
        props.save();
    }
    QVERIFY(!ViewProperties(trashUrl).m_hasStoredProperties);
}

// A storage that will not take the removal keeps what it holds, and the properties say so, rather
// than the folder reading as though it holds nothing while its style is still on disk.
void ViewPropertiesTest::testARemovalThatFailsLeavesTheStyleWrittenDown()
{
#ifndef Q_OS_UNIX
    QSKIP("Only unix is supported, for this test");
#else
    if (getuid() == 0) {
        QSKIP("Running as root - permission checks are not enforced");
    }

    const QUrl trashUrl(QStringLiteral("trash:///"));
    const QString storePath = ViewProperties(trashUrl).m_filePath;
    QVERIFY(QDir().mkpath(storePath));
    {
        ViewProperties props(trashUrl);
        props.setViewMode(DolphinView::CompactView);
        props.save();
    }
    QVERIFY(ViewProperties(trashUrl).m_hasStoredProperties);

    const QFileDevice::Permissions writable = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner;
    QVERIFY(QFile::setPermissions(storePath, QFileDevice::ReadOwner | QFileDevice::ExeOwner));
    auto cleanup = qScopeGuard([storePath, writable, trashUrl] {
        QFile::setPermissions(storePath, writable);
        ViewProperties(trashUrl).forgetStoredProperties();
    });

    ViewProperties props(trashUrl);
    if (props.forgetStoredProperties()) {
        QSKIP("this storage takes a removal from a folder it does not let us write");
    }
    QVERIFY(props.m_hasStoredProperties);
#endif
}

QTEST_GUILESS_MAIN(ViewPropertiesTest)

#include "viewpropertiestest.moc"
