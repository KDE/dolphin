/*
 * SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QCommandLineParser>
#include <QMimeDatabase>
#include <QUrl>
#include <QGuiApplication>
#include <KLocalizedString>
#include <KShell>

#include "../../../config-packagekit.h"

Q_GLOBAL_STATIC_WITH_ARGS(QStringList, binaryPackages, ({QLatin1String("application/vnd.debian.binary-package"),
                                                        QLatin1String("application/x-rpm"),
                                                        QLatin1String("application/x-xz"),
                                                        QLatin1String("application/zstd")}))

enum PackageOperation {
    Install,
    Uninstall
};

#ifdef HAVE_PACKAGEKIT
#include <PackageKit/Daemon>
#include <PackageKit/Details>
#include <PackageKit/Transaction>
#else
#include <QDesktopServices>
#endif

// @param msg Error that gets logged to CLI
Q_NORETURN void fail(const QString &str)
{
    qCritical() << str;
    const QStringList args = {"--detailederror" ,i18n("Dolphin service menu installation failed"),  str};
    QProcess::startDetached("kdialog", args);

    exit(1);
}

QString getServiceMenusDir()
{
    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return QDir(dataLocation).absoluteFilePath("kservices5/ServiceMenus");
}

#ifdef HAVE_PACKAGEKIT
void packageKitInstall(const QString &fileName)
{
    PackageKit::Transaction *transaction = PackageKit::Daemon::installFile(fileName, PackageKit::Transaction::TransactionFlagNone);

    const auto exitWithError = [=](PackageKit::Transaction::Error, const QString &details) {
       fail(details);
    };

    QObject::connect(transaction, &PackageKit::Transaction::finished,
                     [=](PackageKit::Transaction::Exit status, uint) {
                        if (status == PackageKit::Transaction::ExitSuccess) {
                            exit(0);
                        }
                        // Fallback error handling
                        QTimer::singleShot(500, [=](){
                            fail(i18n("Failed to install \"%1\", exited with status \"%2\"",
                                      fileName, QVariant::fromValue(status).toString()));
                        });
                    });
    QObject::connect(transaction, &PackageKit::Transaction::errorCode, exitWithError);
}

void packageKitUninstall(const QString &fileName)
{
    const auto exitWithError = [=](PackageKit::Transaction::Error, const QString &details) {
        fail(details);
    };
    const auto uninstallLambda = [=](PackageKit::Transaction::Exit status, uint) {
        if (status == PackageKit::Transaction::ExitSuccess) {
            exit(0);
        }
    };

    PackageKit::Transaction *transaction = PackageKit::Daemon::getDetailsLocal(fileName);
    QObject::connect(transaction, &PackageKit::Transaction::details,
                     [=](const PackageKit::Details &details) {
                         PackageKit::Transaction *transaction = PackageKit::Daemon::removePackage(details.packageId());
                         QObject::connect(transaction, &PackageKit::Transaction::finished, uninstallLambda);
                         QObject::connect(transaction, &PackageKit::Transaction::errorCode, exitWithError);
                     });

    QObject::connect(transaction, &PackageKit::Transaction::errorCode, exitWithError);
    // Fallback error handling
    QObject::connect(transaction, &PackageKit::Transaction::finished,
        [=](PackageKit::Transaction::Exit status, uint) {
            if (status != PackageKit::Transaction::ExitSuccess) {
                QTimer::singleShot(500, [=]() {
                    fail(i18n("Failed to uninstall \"%1\", exited with status \"%2\"",
                              fileName, QVariant::fromValue(status).toString()));
                });
            }
        });
    }
#endif

Q_NORETURN void packageKit(PackageOperation operation, const QString &fileName)
{
#ifdef HAVE_PACKAGEKIT
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists()) {
        fail(i18n("The file does not exist!"));
    }
    const QString absPath = fileInfo.absoluteFilePath();
    if (operation == PackageOperation::Install) {
        packageKitInstall(absPath);
    } else {
        packageKitUninstall(absPath);
    }
    QGuiApplication::exec(); // For event handling, no return after signals finish
    fail(i18n("Unknown error when installing package"));
#else
    Q_UNUSED(operation)
    QDesktopServices::openUrl(QUrl(fileName));
    exit(0);
#endif
}

struct UncompressCommand
{
    QString command;
    QStringList args1;
    QStringList args2;
};

enum ScriptExecution{
    Process,
    Konsole
};

void runUncompress(const QString &inputPath, const QString &outputPath)
{
    QVector<QPair<QStringList, UncompressCommand>> mimeTypeToCommand;
    mimeTypeToCommand.append({{"application/x-tar", "application/tar", "application/x-gtar", "multipart/x-tar"},
                              UncompressCommand({"tar", {"-xf"}, {"-C"}})});
    mimeTypeToCommand.append({{"application/x-gzip", "application/gzip",
                               "application/x-gzip-compressed-tar", "application/gzip-compressed-tar",
                               "application/x-gzip-compressed", "application/gzip-compressed",
                               "application/tgz", "application/x-compressed-tar",
                               "application/x-compressed-gtar", "file/tgz",
                               "multipart/x-tar-gz", "application/x-gunzip", "application/gzipped",
                               "gzip/document"},
                              UncompressCommand({"tar", {"-zxf"}, {"-C"}})});
    mimeTypeToCommand.append({{"application/bzip", "application/bzip2", "application/x-bzip",
                               "application/x-bzip2", "application/bzip-compressed",
                               "application/bzip2-compressed", "application/x-bzip-compressed",
                               "application/x-bzip2-compressed", "application/bzip-compressed-tar",
                               "application/bzip2-compressed-tar", "application/x-bzip-compressed-tar",
                               "application/x-bzip2-compressed-tar", "application/x-bz2"},
                              UncompressCommand({"tar", {"-jxf"}, {"-C"}})});
    mimeTypeToCommand.append({{"application/zip", "application/x-zip", "application/x-zip-compressed",
                               "multipart/x-zip"},
                              UncompressCommand({"unzip", {}, {"-d"}})});

    const auto mime = QMimeDatabase().mimeTypeForFile(inputPath).name();

    UncompressCommand command{};
    for (const auto &pair : qAsConst(mimeTypeToCommand)) {
        if (pair.first.contains(mime)) {
            command = pair.second;
            break;
        }
    }

    if (command.command.isEmpty()) {
        fail(i18n("Unsupported archive type %1: %2", mime, inputPath));
    }

    QProcess process;
    process.start(
        command.command,
        QStringList() << command.args1 << inputPath << command.args2 << outputPath,
        QIODevice::NotOpen);
    if (!process.waitForStarted()) {
        fail(i18n("Failed to run uncompressor command for %1", inputPath));
    }

    if (!process.waitForFinished()) {
        fail(
            i18n("Process did not finish in reasonable time: %1 %2", process.program(), process.arguments().join(" ")));
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        fail(i18n("Failed to uncompress %1", inputPath));
    }
}

QString findRecursive(const QString &dir, const QString &basename)
{
    QDirIterator it(dir, QStringList{basename}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        return QFileInfo(it.next()).absoluteFilePath();
    }

    return QString();
}

bool runScriptOnce(const QString &path, const QStringList &args, ScriptExecution execution)
{
    QProcess process;
    process.setWorkingDirectory(QFileInfo(path).absolutePath());

    const static bool konsoleAvailable = !QStandardPaths::findExecutable("konsole").isEmpty();
    if (konsoleAvailable && execution == ScriptExecution::Konsole) {
        QString bashCommand = KShell::quoteArg(path) + ' ';
        if (!args.isEmpty()) {
            bashCommand.append(args.join(' '));
        }
        bashCommand.append("|| $SHELL");
        // If the install script fails a shell opens and the user can fix the problem
        // without an error konsole closes
        process.start("konsole", QStringList() << "-e" << "bash" << "-c" << bashCommand, QIODevice::NotOpen);
    } else {
        process.start(path, args, QIODevice::NotOpen);
    }
    if (!process.waitForStarted()) {
        fail(i18n("Failed to run installer script %1", path));
    }

    // Wait until installer exits, without timeout
    if (!process.waitForFinished(-1)) {
        qWarning() << "Failed to wait on installer:" << process.program() << process.arguments().join(" ");
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << "Installer script exited with error:" << process.program() << process.arguments().join(" ");
        return false;
    }

    return true;
}

// If hasArgVariants is true, run "path".
// If hasArgVariants is false, run "path argVariants[i]" until successful.
bool runScriptVariants(const QString &path, bool hasArgVariants, const QStringList &argVariants, QString &errorText)
{
    QFile file(path);
    if (!file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner)) {
        errorText = i18n("Failed to set permissions on %1: %2", path, file.errorString());
        return false;
    }

    qInfo() << "[servicemenuinstaller]: Trying to run installer/uninstaller" << path;
    if (hasArgVariants) {
        for (const auto &arg : argVariants) {
            if (runScriptOnce(path, {arg}, ScriptExecution::Process)) {
                return true;
            }
        }
    } else if (runScriptOnce(path, {}, ScriptExecution::Konsole)) {
        return true;
    }

    errorText = i18nc(
        "%2 = comma separated list of arguments",
        "Installer script %1 failed, tried arguments \"%2\".", path, argVariants.join(i18nc("Separator between arguments", "\", \"")));
    return false;
}

QString generateDirPath(const QString &archive)
{
    return QStringLiteral("%1-dir").arg(archive);
}

bool cmdInstall(const QString &archive, QString &errorText)
{
    const auto serviceDir = getServiceMenusDir();
    if (!QDir().mkpath(serviceDir)) {
        // TODO Cannot get error string because of this bug: https://bugreports.qt.io/browse/QTBUG-1483
        errorText = i18n("Failed to create path %1", serviceDir);
        return false;
    }

    if (archive.endsWith(QLatin1String(".desktop"))) {
        // Append basename to destination directory
        const auto dest = QDir(serviceDir).absoluteFilePath(QFileInfo(archive).fileName());
        if (QFileInfo::exists(dest)) {
            QFile::remove(dest);
        }
        qInfo() << "Single-File Service-Menu" << archive << dest;

        QFile source(archive);
        if (!source.copy(dest)) {
            errorText = i18n("Failed to copy .desktop file %1 to %2: %3", archive, dest, source.errorString());
            return false;
        }
    } else {
        if (binaryPackages->contains(QMimeDatabase().mimeTypeForFile(archive).name())) {
            packageKit(PackageOperation::Install, archive);
        }
        const QString dir = generateDirPath(archive);
        if (QFile::exists(dir)) {
            if (!QDir(dir).removeRecursively()) {
                errorText = i18n("Failed to remove directory %1", dir);
                return false;
            }
        }

        if (QDir().mkdir(dir)) {
            errorText = i18n("Failed to create directory %1", dir);
        }

        runUncompress(archive, dir);

        // Try "install-it" first
        QString installItPath;
        const QStringList basenames1 = {"install-it.sh", "install-it"};
        for (const auto &basename : basenames1) {
            const auto path = findRecursive(dir, basename);
            if (!path.isEmpty()) {
                installItPath = path;
                break;
            }
        }

        if (!installItPath.isEmpty()) {
            return runScriptVariants(installItPath, false, QStringList{}, errorText);
        }

        // If "install-it" is missing, try "install"
        QString installerPath;
        const QStringList basenames2 = {"installKDE4.sh", "installKDE4", "install.sh", "install"};
        for (const auto &basename : basenames2) {
            const auto path = findRecursive(dir, basename);
            if (!path.isEmpty()) {
                installerPath = path;
                break;
            }
        }

        if (!installerPath.isEmpty()) {
            // Try to run script without variants first
            if (!runScriptVariants(installerPath, false, {}, errorText)) {
                return runScriptVariants(installerPath, true, {"--local", "--local-install", "--install"}, errorText);
            }
            return true;
        }

        fail(i18n("Failed to find an installation script in %1", dir));
    }

    return true;
}

bool cmdUninstall(const QString &archive, QString &errorText)
{
    const auto serviceDir = getServiceMenusDir();
    if (archive.endsWith(QLatin1String(".desktop"))) {
        // Append basename to destination directory
        const auto dest = QDir(serviceDir).absoluteFilePath(QFileInfo(archive).fileName());
        QFile file(dest);
        if (!file.remove()) {
            errorText = i18n("Failed to remove .desktop file %1: %2", dest, file.errorString());
            return false;
        }
    } else {
        if (binaryPackages->contains(QMimeDatabase().mimeTypeForFile(archive).name())) {
            packageKit(PackageOperation::Uninstall, archive);
        }
        const QString dir = generateDirPath(archive);

        // Try "deinstall" first
        QString deinstallPath;
        const QStringList basenames1 = {"uninstall.sh", "uninstal", "deinstall.sh", "deinstall"};
        for (const auto &basename : basenames1) {
            const auto path = findRecursive(dir, basename);
            if (!path.isEmpty()) {
                deinstallPath = path;
                break;
            }
        }

        if (!deinstallPath.isEmpty()) {
            const bool ok = runScriptVariants(deinstallPath, false, {}, errorText);
            if (!ok) {
                return ok;
            }
        } else {
            // If "deinstall" is missing, try "install --uninstall"
            QString installerPath;
            const QStringList basenames2 = {"install-it.sh", "install-it", "installKDE4.sh",
                                            "installKDE4", "install.sh", "install"};
            for (const auto &basename : basenames2) {
                const auto path = findRecursive(dir, basename);
                if (!path.isEmpty()) {
                    installerPath = path;
                    break;
                }
            }

            if (!installerPath.isEmpty()) {
                const bool ok = runScriptVariants(installerPath, true,
                                                  {"--remove", "--delete", "--uninstall", "--deinstall"}, errorText);
                if (!ok) {
                    return ok;
                }
            } else {
                fail(i18n("Failed to find an uninstallation script in %1", dir));
            }
        }

        QDir dirObject(dir);
        if (!dirObject.removeRecursively()) {
            errorText = i18n("Failed to remove directory %1", dir);
            return false;
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("command"), i18nc("@info:shell", "Command to execute: install or uninstall."));
    parser.addPositionalArgument(QStringLiteral("path"), i18nc("@info:shell", "Path to archive."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.isEmpty()) {
        fail(i18n("Command is required."));
    }
    if (args.size() == 1) {
        fail(i18n("Path to archive is required."));
    }

    const QString cmd = args[0];
    const QString archive = args[1];

    QString errorText;
    if (cmd == QLatin1String("install")) {
        if (!cmdInstall(archive, errorText)) {
            fail(errorText);
        }
    } else if (cmd == QLatin1String("uninstall")) {
        if (!cmdUninstall(archive, errorText)) {
            fail(errorText);
        }
    } else {
        fail(i18n("Unsupported command %1", cmd));
    }

    return 0;
}
