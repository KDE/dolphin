/***************************************************************************
 *   Copyright © 2019 Alexander Potashev <aspotashev@gmail.com>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <QDebug>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QCommandLineParser>

#include <KLocalizedString>

// @param msg Error that gets logged to CLI
Q_NORETURN void fail(const QString &str)
{
    qCritical() << str;

    QProcess process;
    auto args = QStringList{"--passivepopup", i18n("Dolphin service menu installation failed"), "15"};
    process.start("kdialog", args, QIODevice::ReadOnly);
    if (!process.waitForStarted()) {
        qFatal("Failed to run kdialog");
    }

    exit(1);
}

bool evaluateShell(const QString &program, const QStringList &arguments, QString &output, QString &errorText)
{
    QProcess process;
    process.start(program, arguments, QIODevice::ReadOnly);
    if (!process.waitForStarted()) {
        fail(i18n("Failed to run process: %1 %2", program, arguments.join(" ")));
    }

    if (!process.waitForFinished()) {
        fail(i18n("Process did not finish in reasonable time: %1 %2", program, arguments.join(" ")));
    }

    const auto stdoutResult = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const auto stderrResult = QString::fromUtf8(process.readAllStandardError()).trimmed();

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        output = stdoutResult;
        return true;
    } else {
        errorText = stderrResult + stdoutResult;
        return false;
    }
}

QString mimeType(const QString &path)
{
    QString result;
    QString errorText;
    if (evaluateShell("xdg-mime", QStringList{"query", "filetype", path}, result, errorText)) {
        return result;
    } else {
        fail(i18n("Failed to run xdg-mime %1: %2", path, errorText));
    }
}

QString getServiceMenusDir()
{
    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return QDir(dataLocation).absoluteFilePath("kservices5/ServiceMenus");
}

struct UncompressCommand
{
    QString command;
    QStringList args1;
    QStringList args2;
};

void runUncompress(const QString &inputPath, const QString &outputPath) {
    QList<QPair<QStringList, UncompressCommand>> mimeTypeToCommand;
    mimeTypeToCommand.append({QStringList{"application/x-tar", "application/tar", "application/x-gtar",
                                          "multipart/x-tar"},
                              UncompressCommand{"tar", QStringList() << "-xf", QStringList() << "-C"}});
    mimeTypeToCommand.append({QStringList{"application/x-gzip", "application/gzip",
                                          "application/x-gzip-compressed-tar", "application/gzip-compressed-tar",
                                          "application/x-gzip-compressed", "application/gzip-compressed",
                                          "application/tgz", "application/x-compressed-tar",
                                          "application/x-compressed-gtar", "file/tgz",
                                          "multipart/x-tar-gz", "application/x-gunzip", "application/gzipped",
                                          "gzip/document"},
                              UncompressCommand{"tar", QStringList{"-zxf"}, QStringList{"-C"}}});
    mimeTypeToCommand.append({QStringList{"application/bzip", "application/bzip2", "application/x-bzip",
                                          "application/x-bzip2", "application/bzip-compressed",
                                          "application/bzip2-compressed", "application/x-bzip-compressed",
                                          "application/x-bzip2-compressed", "application/bzip-compressed-tar",
                                          "application/bzip2-compressed-tar", "application/x-bzip-compressed-tar",
                                          "application/x-bzip2-compressed-tar", "application/x-bz2"},
                              UncompressCommand{"tar", QStringList{"-jxf"}, QStringList{"-C"}}});
    mimeTypeToCommand.append({QStringList{"application/zip", "application/x-zip", "application/x-zip-compressed",
                                          "multipart/x-zip"},
                              UncompressCommand{"unzip", QStringList{}, QStringList{"-d"}}});

    const auto mime = mimeType(inputPath);

    UncompressCommand command{};
    for (const auto &pair : mimeTypeToCommand) {
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
        return QFileInfo(it.next()).canonicalFilePath();
    }

    return QString();
}

bool runInstallerScriptOnce(const QString &path, const QStringList &args)
{
    QProcess process;
    process.setWorkingDirectory(QFileInfo(path).absolutePath());

    process.start(path, args, QIODevice::NotOpen);
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
bool runInstallerScript(const QString &path, bool hasArgVariants, const QStringList &argVariants, QString &errorText)
{
    QFile file(path);
    if (!file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner)) {
        errorText = i18n("Failed to set permissions on %1: %2", path, file.errorString());
        return false;
    }

    qInfo() << "[servicemenuinstaller]: Trying to run installer/uninstaller" << path;
    if (hasArgVariants) {
        for (const auto &arg : argVariants) {
            if (runInstallerScriptOnce(path, QStringList{arg})) {
                return true;
            }
        }
    } else {
        if (runInstallerScriptOnce(path, QStringList{})) {
            return true;
        }
    }

    errorText = i18nc(
        "%1 = comma separated list of arguments",
        "Installer script %1 failed, tried arguments \"%1\".", path, argVariants.join(i18nc("Separator between arguments", "\", \"")));
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

    if (archive.endsWith(".desktop")) {
        // Append basename to destination directory
        const auto dest = QDir(serviceDir).absoluteFilePath(QFileInfo(archive).fileName());
        qInfo() << "Single-File Service-Menu" << archive << dest;

        QFile source(archive);
        if (!source.copy(dest)) {
            errorText = i18n("Failed to copy .desktop file %1 to %2: %3", archive, dest, source.errorString());
            return false;
        }
    } else {
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
        const auto basenames1 = QStringList{"install-it.sh", "install-it"};
        for (const auto &basename : qAsConst(basenames1)) {
            const auto path = findRecursive(dir, basename);
            if (!path.isEmpty()) {
                installItPath = path;
                break;
            }
        }

        if (!installItPath.isEmpty()) {
            return runInstallerScript(installItPath, false, QStringList{}, errorText);
        }

        // If "install-it" is missing, try "install"
        QString installerPath;
        const auto basenames2 = QStringList{"installKDE4.sh", "installKDE4", "install.sh", "install"};
        for (const auto &basename : qAsConst(basenames2)) {
            const auto path = findRecursive(dir, basename);
            if (!path.isEmpty()) {
                installerPath = path;
                break;
            }
        }

        if (!installerPath.isEmpty()) {
            return runInstallerScript(installerPath, true, QStringList{"--local", "--local-install", "--install"}, errorText);
        }

        fail(i18n("Failed to find an installation script in %1", dir));
    }

    return true;
}

bool cmdUninstall(const QString &archive, QString &errorText)
{
    const auto serviceDir = getServiceMenusDir();
    if (archive.endsWith(".desktop")) {
        // Append basename to destination directory
        const auto dest = QDir(serviceDir).absoluteFilePath(QFileInfo(archive).fileName());
        QFile file(dest);
        if (!file.remove()) {
            errorText = i18n("Failed to remove .desktop file %1: %2", dest, file.errorString());
            return false;
        }
    } else {
        const QString dir = generateDirPath(archive);

        // Try "deinstall" first
        QString deinstallPath;
        const auto basenames1 = QStringList{"deinstall.sh", "deinstall"};
        for (const auto &basename : qAsConst(basenames1)) {
            const auto path = findRecursive(dir, basename);
            if (!path.isEmpty()) {
                deinstallPath = path;
                break;
            }
        }

        if (!deinstallPath.isEmpty()) {
            bool ok = runInstallerScript(deinstallPath, false, QStringList{}, errorText);
            if (!ok) {
                return ok;
            }
        } else {
            // If "deinstall" is missing, try "install --uninstall"

            QString installerPath;
            const auto basenames2 = QStringList{"install-it.sh", "install-it", "installKDE4.sh",
                                                "installKDE4", "install.sh", "install"};
            for (const auto &basename : qAsConst(basenames2)) {
                const auto path = findRecursive(dir, basename);
                if (!path.isEmpty()) {
                    installerPath = path;
                    break;
                }
            }

            if (!installerPath.isEmpty()) {
                bool ok = runInstallerScript(
                    installerPath, true, QStringList{"--remove", "--delete", "--uninstall", "--deinstall"}, errorText);
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
    QCoreApplication app(argc, argv);

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
    if (cmd == "install") {
        if (!cmdInstall(archive, errorText)) {
            fail(errorText);
        }
    } else if (cmd == "uninstall") {
        if (!cmdUninstall(archive, errorText)) {
            fail(errorText);
        }
    } else {
        fail(i18n("Unsupported command %1", cmd));
    }

    return 0;
}
