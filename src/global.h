/*
 * SPDX-FileCopyrightText: 2015 Ashish Bansal <bansal.ashish096@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QList>
#include <QUrl>
#include <QWidget>

class OrgKdeDolphinMainWindowInterface;

namespace Dolphin {
    QList<QUrl> validateUris(const QStringList& uriList);

    /**
     * Returns the home url which is defined in General Settings
     */
    QUrl homeUrl();

    enum class OpenNewWindowFlag {
        None = 0,
        Select = 1<<1
    };
    Q_DECLARE_FLAGS(OpenNewWindowFlags, OpenNewWindowFlag)

    /**
     * Opens a new Dolphin window
     */
    void openNewWindow(const QList<QUrl> &urls = {}, QWidget *window = nullptr, const OpenNewWindowFlags &flags = OpenNewWindowFlag::None);

    /**
     * Attaches URLs to an existing Dolphin instance if possible.
     * If @p preferredService is a valid dbus service, it will be tried first.
     * @p preferredService needs to support the org.kde.dolphin.MainWindow dbus interface with the /dolphin/Dolphin_1 path.
     * Returns true if the URLs were successfully attached.
     */
    bool attachToExistingInstance(const QList<QUrl>& inputUrls, bool openFiles, bool splitView, const QString& preferredService = QString());

    /**
     * Returns a QVector with all GUI-capable Dolphin instances
     */
    QVector<QPair<QSharedPointer<OrgKdeDolphinMainWindowInterface>, QStringList>> dolphinGuiInstances(const QString& preferredService);

    /**
     * TODO: Move this somewhere global to all KDE apps, not just Dolphin
     */
    const int VERTICAL_SPACER_HEIGHT = 12;
    const int LAYOUT_SPACING_SMALL = 2;
}

#endif //GLOBAL_H
