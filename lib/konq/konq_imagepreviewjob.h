/*  This file is part of the KDE project
    Copyright (C) 2000 David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __konq_imagepreviewjob_h__
#define __konq_imagepreviewjob_h__

#include <kio/job.h>
#include <kservice.h>

class QImage;
class KFileIVI;
class KonqIconViewWidget;

typedef QMap<QString, KService::Ptr> PluginMap;

/**
 * @deprecated
 * A job that determines the thumbnails for the images in the current directory
 * of the icon view (KonqIconViewWidget)
 *
 * Supports png pics and xvpics, and generates png pics when none found.
 * Features network transparency, asynchronous processing, and safety
 * against deleted items.
 * ### KDE 3.0 remove
 */
class KonqImagePreviewJob : public KIO::Job
{
    Q_OBJECT
public:

    /**
     * Create a job for determining the pixmaps of the images in the @p iconView
     * @p transparency is a value from 0..255 determining the transparency of
     * an icon blended into the text-preview. 0 means the icon is completely
     * transparent (invisible), while 255 means it is completely opaque.
     */
    KonqImagePreviewJob( KonqIconViewWidget * iconView, bool force,
			 int transparency, const QStringList &previewSettings );
    virtual ~KonqImagePreviewJob();

    // Call this to get started
    void startImagePreview();

    void itemRemoved( KFileIVI * item );

protected:
    void determineNextIcon();
    void getOrCreateThumbnail();
    bool statResultThumbnail( KIO::StatJob *job = 0 );
    void createThumbnail( QString );

protected slots:
    virtual void slotResult( KIO::Job *job );

private slots:
    void slotThumbData(KIO::Job *, const QByteArray &);

private:
    void saveThumbnail(const QByteArray &imgData);

private:
    enum { STATE_STATXVDIR, // one-time check for .xvpics dir
           STATE_STATORIG, STATE_STATXV, STATE_GETTHUMB, // if the thumbnail exists
           STATE_GETORIG, // if we create it
           STATE_CREATETHUMB // thumbnail:/ slave
    } m_state;

    // Our todo list :)
    QList<KFileIVI> m_items;

    // The current item
    KFileIVI *m_currentItem;
    // The URL of the current item (always equivalent to m_items.first()->item()->url())
    KURL m_currentURL;
    // true if an .xvpics subdir exists
    bool m_bHasXvpics;
    // The modification time of that URL
    time_t m_tOrig;
    // Path to thumbnail cache for this directory
    QString m_thumbPath;
    // Size of thumbnail
    int m_extent;
    // Whether we can save the thumbnail
    bool m_bCanSave;
    // If the file to create a thumb for was a temp file, this is its name
    QString m_tempName;

    // Dad :)
    KonqIconViewWidget * m_iconView;

    // Over that, it's too much
    unsigned long m_maximumSize;

    // the transparency of the blended mimetype icon
    // {0..255}, shifted into the upper 8 bits
    int m_transparency;
    // Plugin service cache
    PluginMap m_plugins;
	// Shared memory segment Id. The segment is allocated to a size
	// of extent x extent x 4 (32 bit image) on first need.
	int m_shmid;
	// And the data area
	char *m_shmaddr;
};

#endif
