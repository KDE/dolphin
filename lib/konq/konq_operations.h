/*  This file is part of the KDE project
    Copyright (C) 2000  David Faure <faure@kde.org>

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

#ifndef __konq_operations_h__
#define __konq_operations_h__

#include <kurl.h>
#include <qobject.h>
#include <konq_fileitem.h>

namespace KIO { class Job; }
class QWidget;

/**
 * Implements file operations (move,del,trash,shred,paste,copy,move,link...)
 * for konqueror and kdesktop whatever the view mode is (icon, tree, ...)
 */
class KonqOperations : public QObject
{
    Q_OBJECT
protected:
    KonqOperations( QWidget * parent );
    virtual ~KonqOperations() {}

public:
    /**
     * Pop up properties dialog for mimetype @p mimeType.
     */
    static void editMimeType( const QString & mimeType );

    enum { TRASH, DEL, SHRED, COPY, MOVE, LINK };
    /**
     * @param method should be TRASH, DEL or SHRED
     */
    static void del( QWidget * parent, int method, const KURL::List & selectedURLs );
    /**
     * Drop
     * @param destItem destination KonqFileItem for the drop (background or item)
     * @param ev the drop event
     * @param parent parent widget (for error dialog box if any)
     */
    static void doDrop( const KonqFileItem * destItem, QDropEvent * ev, QWidget * parent );

    static void emptyTrash();

protected:
    enum { DEFAULT_CONFIRMATION, SKIP_CONFIRMATION, FORCE_CONFIRMATION };
    bool askDeleteConfirmation( const KURL::List & selectedURLs, int confirmation );
    void _del( int method, const KURL::List & selectedURLs, int confirmation );

    // internal, for COPY/MOVE/LINK
    void setOperation( KIO::Job * job, int method, const KURL::List & src, const KURL & dest );

protected slots:

    void slotResult( KIO::Job * job );

private:
    int m_method;
    KURL::List m_srcURLs;
    KURL m_destURL;
};

#endif
