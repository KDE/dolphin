/* This file is part of the KDE Project
   Copyright (c) 2001 Malte Starostik <malte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _FAVICONS_H_
#define _FAVICONS_H_

#include <kdedmodule.h>
#include <kurl.h>

class KJob;
namespace KIO { class Job; }

/**
 * KDED Module to handle shortcut icons ("favicons")
 * FavIconsModule implements a KDED Module that handles the association of
 * URLs and hosts with shortcut icons and the icons' downloads in a central
 * place.
 *
 * After a successful download, the DBUS signal iconChanged() is emitted.
 * It has the signature void iconChanged(bool, QString, QString);
 * The first parameter is true if the icon is a "host" icon, that is it is
 * the default icon for all URLs on the given host. In this case, the
 * second parameter is a host name, otherwise the second parameter is the
 * URL which is associated with the icon. The third parameter is the
 * @ref KIconLoader friendly name of the downloaded icon, the same as
 * @ref iconForURL will from now on return for any matching URL.
 *
 * @short KDED Module for favicons
 * @author Malte Starostik <malte@kde.org>
 */
class FavIconsModule : public KDEDModule
{
    Q_OBJECT
public:
    FavIconsModule();
    virtual ~FavIconsModule();

public Q_SLOTS: // dbus methods
    /**
     * Looks up an icon name for a given URL. This function does not
     * initiate any download. If no icon for the URL or its host has
     * been downloaded yet, QString() is returned.
     *
     * @param url the URL for which the icon is queried
     * @return the icon name suitable to pass to @ref KIconLoader or
     *         QString() if no icon for this URL was found.
     */
    QString iconForURL(const KUrl &url);

    /**
     * Associates an icon with the given URL. If the icon was not
     * downloaded before or the downloaded was too long ago, a
     * download attempt will be started and the iconChanged() DBUS
     * signal is emitted after the download finished successfully.
     *
     * @param url the URL which will be associated with the icon
     * @param iconURL the URL of the icon to be downloaded
     */
    void /*ASYNC*/ setIconForURL(const KUrl &url, const KUrl &iconURL);
    /**
     * Downloads the icon for a given host if it was not downloaded before
     * or the download was too long ago. If the download finishes
     * successfully, the iconChanged() DBUS signal is emitted.
     *
     * @param url any URL on the host for which the icon is to be downloaded
     */
    void /*ASYNC*/ downloadHostIcon(const KUrl &url);

signals: // DBUS signals
    void iconChanged(bool isHost, QString hostOrURL, QString iconName);
    void infoMessage(QString iconURL, QString msg);

private:
    void startDownload(const QString &, bool, const KUrl &);
    QString simplifyURL(const KUrl &);
    QString iconNameFromURL(const KUrl &);
    bool isIconOld(const QString &);

private Q_SLOTS:
    void slotData(KIO::Job *, const QByteArray &);
    void slotResult(KJob *);
    void slotInfoMessage(KJob *, const QString &);
    void slotKill();

private:
    struct FavIconsModulePrivate *d;
};

#endif

// vim: ts=4 sw=4 et
