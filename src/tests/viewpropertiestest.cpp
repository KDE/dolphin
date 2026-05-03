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
    void testRestoreViewProps();

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
    const QString destinationDir = props->destinationDir(QStringLiteral("local")) + localFolder;

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

QTEST_GUILESS_MAIN(ViewPropertiesTest)

#include "viewpropertiestest.moc"
