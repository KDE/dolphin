/* This file is part of the KDE projects
   Copyright (C) 2005 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQMIMEDATA_H
#define KONQMIMEDATA_H

#include <libkonq_export.h>
#include <kurl.h>
class QMimeData;

// Clipboard/dnd data for: (kde) urls, most-local urls, isCut
class LIBKONQ_EXPORT KonqMimeData
{
public:
    /**
     * Populate a QMimeData with urls, and whether they were cut or copied.
     *
     * @param kdeURLs list of urls (which can include kde-specific protocols).
     * This list can be empty if only local urls are being used anyway.
     * @param mostLocalURLs "most local urls" (which try to resolve those to file:/ where possible),
     * @param cut if true, the user selected "cut" (saved as application/x-kde-cutselection in the mimedata).
     */
    static void populateMimeData( QMimeData* mimeData,
                                  const KUrl::List& kdeURLs,
                                  const KUrl::List& mostLocalURLs,
                                  bool cut = false );

    // TODO other methods for icon positions

    /**
     * @return true if the urls in @p mimeData were cut
     */
    static bool decodeIsCutSelection( const QMimeData *mimeData );
};

#endif /* KONQMIMEDATA_H */

