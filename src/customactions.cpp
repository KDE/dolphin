/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "customactions.h"

#include <KConfigGroup>
#include <KIO/CommandLauncherJob>
#include <KService>
#include <KSharedConfig>
#include <KShell>

#include <QFileInfo>
#include <QRegularExpression>
#include <QWidget> // the launcher job takes a QObject parent, so the type must be complete

namespace
{
QString targetName(CustomActions::Target target)
{
    return target == CustomActions::Target::Directories ? QStringLiteral("Directories") : QStringLiteral("Files");
}

KConfigGroup listGroup(CustomActions::Target target)
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    return config->group(QStringLiteral("CustomActions")).group(targetName(target));
}

QString quotedPath(const QUrl &url)
{
    return KShell::quoteArg(url.isLocalFile() ? url.toLocalFile() : url.toString());
}

QString joinedPaths(const QList<QUrl> &urls)
{
    QStringList quoted;
    quoted.reserve(urls.count());
    for (const QUrl &url : urls) {
        quoted.append(quotedPath(url));
    }
    return quoted.join(QLatin1Char(' '));
}
}

QList<CustomActions::Entry> CustomActions::load(Target target)
{
    QList<Entry> entries;

    const KConfigGroup group = listGroup(target);
    const int count = group.readEntry("Count", 0);
    entries.reserve(count);

    for (int i = 0; i < count; ++i) {
        const KConfigGroup stored = group.group(QString::number(i));
        Entry entry;
        entry.name = stored.readEntry("Name", QString());
        entry.icon = stored.readEntry("Icon", QString());
        entry.command = stored.readEntry("Command", QString());
        if (!entry.name.isEmpty() && !entry.command.isEmpty()) {
            entries.append(entry);
        }
    }

    return entries;
}

void CustomActions::save(Target target, const QList<Entry> &entries)
{
    KConfigGroup group = listGroup(target);

    // Entries may have been removed or reordered, so drop the old numbering.
    const int previous = group.readEntry("Count", 0);
    for (int i = 0; i < previous; ++i) {
        group.deleteGroup(QString::number(i));
    }

    for (int i = 0; i < entries.count(); ++i) {
        KConfigGroup stored = group.group(QString::number(i));
        stored.writeEntry("Name", entries.at(i).name);
        stored.writeEntry("Icon", entries.at(i).icon);
        stored.writeEntry("Command", entries.at(i).command);
    }

    group.writeEntry("Count", entries.count());
    group.sync();
}

QString CustomActions::commandFor(const QString &path, QString *suggestedName, QString *suggestedIcon)
{
    if (path.isEmpty()) {
        return {};
    }

    if (path.endsWith(QLatin1String(".desktop"))) {
        const KService service(path);
        QString exec = service.exec();
        if (exec.isEmpty()) {
            return {};
        }

        // Drop the Exec field codes: our own placeholders take their place.
        static const QRegularExpression fieldCodes(QStringLiteral("%[uUfFickdDnNvm]"));
        exec.remove(fieldCodes);
        exec = exec.simplified();

        if (suggestedName && !service.name().isEmpty()) {
            *suggestedName = service.name();
        }
        if (suggestedIcon && !service.icon().isEmpty()) {
            *suggestedIcon = service.icon();
        }
        return exec;
    }

    return KShell::quoteArg(path);
}

QString CustomActions::replaceProgram(const QString &command, const QString &program)
{
    if (program.isEmpty()) {
        return command;
    }
    if (command.trimmed().isEmpty()) {
        return program;
    }

    KShell::Errors error = KShell::NoError;
    QStringList arguments = KShell::splitArgs(command, KShell::TildeExpand, &error);
    if (error != KShell::NoError || arguments.isEmpty()) {
        // Unparseable so far - the picked program is the safer thing to keep.
        return program;
    }

    arguments.removeFirst();
    if (arguments.isEmpty()) {
        return program;
    }
    return program + QLatin1Char(' ') + KShell::joinArgs(arguments);
}

void CustomActions::run(const Entry &entry, const QList<QUrl> &urls, QWidget *window)
{
    if (entry.command.isEmpty() || urls.isEmpty()) {
        return;
    }

    QString command = entry.command;
    const bool hasPlaceholder = command.contains(QLatin1String("%f")) //
        || command.contains(QLatin1String("%F")) //
        || command.contains(QLatin1String("%u")) //
        || command.contains(QLatin1String("%U")) //
        || command.contains(QLatin1String("%d"));

    const QUrl &first = urls.first();
    command.replace(QLatin1String("%F"), joinedPaths(urls));
    command.replace(QLatin1String("%U"), joinedPaths(urls));
    command.replace(QLatin1String("%f"), quotedPath(first));
    command.replace(QLatin1String("%u"), KShell::quoteArg(first.toString()));
    command.replace(QLatin1String("%d"), KShell::quoteArg(first.adjusted(QUrl::RemoveFilename).toLocalFile()));

    if (!hasPlaceholder) {
        command += QLatin1Char(' ') + joinedPaths(urls);
    }

    auto *job = new KIO::CommandLauncherJob(command, window);
    job->setDesktopName(QStringLiteral("org.kde.dolphin"));
    // Start where the items are, so relative paths in the command behave.
    if (first.isLocalFile()) {
        job->setWorkingDirectory(QFileInfo(first.toLocalFile()).absolutePath());
    }
    job->start();
}
