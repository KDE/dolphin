/*
 * SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
 * SPDX-FileCopyrightText: 2026 Peder Bergan <contact@pederbe.dev>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class ServiceMenuInstallerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void installArchive_data();
    void installArchive();
    void installZipArchive();
    void uninstallArchive_data();
    void uninstallArchive();
    void installDesktopFile();
    void uninstallDesktopFile();

private:
    bool runProcess(const QTemporaryDir &tempDir, const QString &program, const QStringList &arguments, bool expectSuccess = true) const;

    QTemporaryDir m_toolsDir;
};

static bool writeFile(const QString &path, const QByteArray &contents = {})
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size() && file.flush();
}

void ServiceMenuInstallerTest::initTestCase()
{
    QVERIFY(m_toolsDir.isValid());
    const QString tar = QStandardPaths::findExecutable(u"tar"_s);
    QVERIFY2(!tar.isEmpty(), "tar is required to test service menu archives");
    QVERIFY(QFile::link(tar, m_toolsDir.filePath(u"tar"_s)));
    const QString unzip = QStandardPaths::findExecutable(u"unzip"_s);
    if (!unzip.isEmpty()) {
        QVERIFY(QFile::link(unzip, m_toolsDir.filePath(u"unzip"_s)));
    }
}

bool ServiceMenuInstallerTest::runProcess(const QTemporaryDir &tempDir, const QString &program, const QStringList &arguments, bool expectSuccess) const
{
    const QString home = tempDir.filePath(u"home"_s);
    if (!QDir().mkpath(home)) {
        qWarning() << "Could not create the temporary home directory" << home;
        return false;
    }

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(u"HOME"_s, home);
    environment.insert(u"XDG_DATA_HOME"_s, tempDir.path());
    environment.insert(u"QT_QPA_PLATFORM"_s, u"offscreen"_s);
    environment.remove(u"TAR_OPTIONS"_s);
    environment.remove(u"UNZIP"_s);
    environment.remove(u"UNZIPOPT"_s);
    // Keep the archive tools available without launching the host's Konsole or KDialog.
    environment.insert(u"PATH"_s, m_toolsDir.path());

    QProcess process;
    process.setProcessEnvironment(environment);
    process.setWorkingDirectory(tempDir.path());
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(program, arguments);
    if (!process.waitForFinished()) {
        qWarning() << program << arguments << process.errorString();
        return false;
    }
    const bool exitAsExpected = process.exitStatus() == QProcess::NormalExit && (process.exitCode() == 0) == expectSuccess;
    if (!exitAsExpected) {
        qWarning() << program << arguments << "expected success:" << expectSuccess << "exit code:" << process.exitCode() << process.exitStatus();
        qWarning().noquote() << QString::fromLocal8Bit(process.readAll());
    }
    return exitAsExpected;
}

void ServiceMenuInstallerTest::installArchive_data()
{
    QTest::addColumn<bool>("hasInstallIt");
    QTest::addColumn<QString>("scriptName");
    QTest::addColumn<QByteArray>("installScript");

    QTest::newRow("prefer-install-it") << true << u"install.sh"_s << QByteArray("#!/bin/sh\n: > install-ran\n");
    QTest::newRow("install-argument") << false << u"install.sh"_s << QByteArray("#!/bin/sh\n[ \"$#\" -eq 1 ] && [ \"$1\" = --install ] && : > install-ran\n");
    QTest::newRow("wildcard-script") << false << u"install-menu.sh"_s << QByteArray("#!/bin/sh\n: > install-ran\n");
    QTest::newRow("missing-script") << false << u"install.sh"_s << QByteArray();
}

void ServiceMenuInstallerTest::installArchive()
{
    QFETCH(bool, hasInstallIt);
    QFETCH(QString, scriptName);
    QFETCH(QByteArray, installScript);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QVERIFY(QDir(tempDir.path()).mkdir(u"menu"_s));
    if (hasInstallIt) {
        QVERIFY(writeFile(tempDir.filePath(u"menu/install-it.sh"_s), "#!/bin/sh\n: > install-it-ran\n"));
    }
    if (!installScript.isEmpty()) {
        QVERIFY(writeFile(tempDir.filePath(u"menu/"_s + scriptName), installScript));
    }

    const QString archive = tempDir.filePath(u"menu.tar"_s);
    QVERIFY(runProcess(tempDir, m_toolsDir.filePath(u"tar"_s), {u"-cf"_s, archive, u"menu"_s}));
    QVERIFY(QFile::exists(archive));
    const bool hasScript = hasInstallIt || !installScript.isEmpty();
    QVERIFY(runProcess(tempDir, QStringLiteral(SERVICEMENUINSTALLER_PATH), {u"install"_s, archive}, hasScript));

    const QDir extractedDir(archive + u"-dir/menu"_s);
    QVERIFY(extractedDir.exists());
    QCOMPARE(extractedDir.exists(u"install-it.sh"_s), hasInstallIt);
    QCOMPARE(extractedDir.exists(scriptName), !installScript.isEmpty());
    QCOMPARE(extractedDir.exists(u"install-it-ran"_s), hasInstallIt);
    QCOMPARE(extractedDir.exists(u"install-ran"_s), !hasInstallIt && !installScript.isEmpty());
}

void ServiceMenuInstallerTest::installZipArchive()
{
    if (!QFile::exists(m_toolsDir.filePath(u"unzip"_s))) {
        QSKIP("unzip is required to test ZIP service menu archives");
    }

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString scriptName = u"install.sh"_s;
    QVERIFY(writeFile(tempDir.filePath(scriptName), "#!/bin/sh\n: > install-ran\n"));
    const QString archive = tempDir.filePath(u"menu.zip"_s);

    QVERIFY(runProcess(tempDir, QStringLiteral(CMAKE_COMMAND_PATH), {u"-E"_s, u"tar"_s, u"cf"_s, archive, u"--format=zip"_s, u"--"_s, scriptName}));
    QVERIFY(runProcess(tempDir, QStringLiteral(SERVICEMENUINSTALLER_PATH), {u"install"_s, archive}));

    QVERIFY(QFile::exists(archive + u"-dir/install-ran"_s));
}

void ServiceMenuInstallerTest::uninstallArchive_data()
{
    QTest::addColumn<QString>("removeScriptName");
    QTest::addColumn<QByteArray>("installScript");

    QTest::newRow("prefer-deinstall") << u"deinstall.sh"_s << QByteArray("#!/bin/sh\n: > \"$XDG_DATA_HOME/install-ran\"\n");
    QTest::newRow("prefer-uninstall") << u"uninstall.sh"_s << QByteArray("#!/bin/sh\n: > \"$XDG_DATA_HOME/install-ran\"\n");
    QTest::newRow("uninstall-argument") << QString()
                                        << QByteArray("#!/bin/sh\n[ \"$#\" -eq 1 ] && [ \"$1\" = --uninstall ] && : > \"$XDG_DATA_HOME/install-ran\"\n");
    QTest::newRow("missing-script") << QString() << QByteArray();
}

void ServiceMenuInstallerTest::uninstallArchive()
{
    QFETCH(QString, removeScriptName);
    QFETCH(QByteArray, installScript);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString archive = tempDir.filePath(u"menu.zip"_s);
    const QString extractedPath = archive + u"-dir"_s;
    const QDir scriptDir(extractedPath + u"/menu"_s);
    QVERIFY(QDir().mkpath(scriptDir.path()));
    const bool hasRemoveScript = !removeScriptName.isEmpty();
    if (hasRemoveScript) {
        QVERIFY(writeFile(scriptDir.filePath(removeScriptName), "#!/bin/sh\n[ -f \"${0##*/}\" ] && : > \"$XDG_DATA_HOME/remove-ran\"\n"));
    }
    if (!installScript.isEmpty()) {
        QVERIFY(writeFile(scriptDir.filePath(u"install.sh"_s), installScript));
    }

    const bool hasScript = hasRemoveScript || !installScript.isEmpty();
    QVERIFY(runProcess(tempDir, QStringLiteral(SERVICEMENUINSTALLER_PATH), {u"uninstall"_s, archive}, hasScript));

    if (hasScript) {
        QVERIFY(!QDir(extractedPath).exists());
    } else {
        QVERIFY(scriptDir.exists());
    }
    QCOMPARE(QFile::exists(tempDir.filePath(u"remove-ran"_s)), hasRemoveScript);
    QCOMPARE(QFile::exists(tempDir.filePath(u"install-ran"_s)), !hasRemoveScript && !installScript.isEmpty());
}

void ServiceMenuInstallerTest::installDesktopFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString downloaded = tempDir.filePath(u"menu.desktop"_s);
    const QByteArray contents("[Desktop Entry]\nType=Service\nName=Test Service Menu\n");
    QVERIFY(writeFile(downloaded, contents));

    QVERIFY(runProcess(tempDir, QStringLiteral(SERVICEMENUINSTALLER_PATH), {u"install"_s, downloaded}));

    QVERIFY(QFile::exists(downloaded));
    QFile installed(tempDir.filePath(u"kio/servicemenus/menu.desktop"_s));
    QVERIFY(installed.open(QIODevice::ReadOnly));
    QCOMPARE(installed.readAll(), contents);
    QVERIFY(installed.permissions().testFlag(QFile::ExeOwner));
}

void ServiceMenuInstallerTest::uninstallDesktopFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString downloaded = tempDir.filePath(u"menu.desktop"_s);
    const QString installed = tempDir.filePath(u"kio/servicemenus/menu.desktop"_s);
    QVERIFY(QDir(tempDir.path()).mkpath(u"kio/servicemenus"_s));
    QVERIFY(writeFile(downloaded));
    QVERIFY(writeFile(installed));

    QVERIFY(runProcess(tempDir, QStringLiteral(SERVICEMENUINSTALLER_PATH), {u"uninstall"_s, downloaded}));

    QVERIFY(QFile::exists(downloaded));
    QVERIFY(!QFile::exists(installed));
}

QTEST_GUILESS_MAIN(ServiceMenuInstallerTest)

#include "servicemenuinstallertest.moc"
