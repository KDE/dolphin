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

class KFileIVI;
class KonqIconViewWidget;

/**
 * A job that determines the thumbnails for the images in the current directory
 * of the icon view (KonqIconViewWidget)
 *
 * Supports png pics and xvpics, and generates png pics when none found.
 * Features network transparency, asynchronous processing, and safety
 * against deleted items.
 */
class KonqImagePreviewJob : public KIO::Job
{
    Q_OBJECT
public:

    /**
     * Create a job for determining the pixmaps of the images in the @p iconView
     */
    KonqImagePreviewJob( KonqIconViewWidget * iconView, bool force );
    virtual ~KonqImagePreviewJob();

    // Call this to get started
    void startImagePreview();

    void itemRemoved( KFileIVI * item );

protected:
    void determineNextIcon();
    void determineThumbnailURL();
    bool statResultThumbnail( KIO::StatJob * );
    void createThumbnail( QString );

protected slots:
    virtual void slotResult( KIO::Job *job );

private:
    enum { STATE_STATORIG, STATE_STATTHUMB, STATE_STATXV, STATE_GETTHUMB, // if the thumbnail exists
           STATE_CREATEDIR1, STATE_CREATEDIR2, STATE_GETORIG, STATE_PUTTHUMB // if we create it
    } m_state;

    // Our todo list :)
    QList<KFileIVI> m_items;

    // The current item
    KFileIVI *m_currentItem;
    // The URL of the current item (always equivalent to m_items.first()->item()->url())
    KURL m_currentURL;
    // The modification time of that URL
    time_t m_tOrig;
    // The URL where we find (or create) the thumbnail for the current URL
    KURL m_thumbURL;
    // Size of thumbnail
    int m_extent;
    // Whether we can save the thumbnail
    bool m_bCanSave;
    // Set to true if we created the dirs - caching
    bool m_bDirsCreated;

    // Dad :)
    KonqIconViewWidget * m_iconView;

    // Over that, it's too much
    unsigned long m_maximumSize;
};

#endif
