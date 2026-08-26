/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef CUSTOMACTIONS_H
#define CUSTOMACTIONS_H

#include <QList>
#include <QString>
#include <QUrl>

class QWidget;

/**
 * @brief User-defined context menu entries.
 *
 * Two separate lists are kept: one that shows up when the selection is made of
 * directories, one for a selection of files. A mixed selection gets neither,
 * since an entry written for one kind rarely makes sense for the other.
 */
namespace CustomActions
{
enum class Target {
    Files,
    Directories,
};

struct Entry {
    QString name;
    QString icon;
    QString command;

    bool operator==(const Entry &other) const
    {
        return name == other.name && icon == other.icon && command == other.command;
    }
};

/** Reads the entries of one list from dolphinrc. */
QList<Entry> load(Target target);

/** Writes one list back to dolphinrc, replacing what was there. */
void save(Target target, const QList<Entry> &entries);

/**
 * Turns a file picked from disk into something to run.
 *
 * A .desktop file yields its Exec line without the field codes, and fills in
 * @p suggestedName and @p suggestedIcon; anything else is taken as an
 * executable and quoted. Returns an empty string if the file is unusable.
 */
QString commandFor(const QString &path, QString *suggestedName = nullptr, QString *suggestedIcon = nullptr);

/**
 * Puts @p program at the front of @p command, keeping the arguments that were
 * already there, so picking an application twice does not lose the parameters.
 */
QString replaceProgram(const QString &command, const QString &program);

/**
 * Runs an entry against the selected items.
 *
 * The command may contain the usual placeholders: %f and %F for a local path
 * and all local paths, %u and %U for a URL and all URLs, %d for the folder the
 * first item is in. A command with no placeholder at all gets the paths
 * appended, so "ark --add" behaves as expected.
 */
void run(const Entry &entry, const QList<QUrl> &urls, QWidget *window);
}

#endif
