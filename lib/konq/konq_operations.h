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
#include <qevent.h>
#include <konq_fileitem.h>

namespace KIO { class Job; }
class QWidget;
class KonqMainWindow;

/**
 * Implements file operations (move,del,trash,shred,paste,copy,move,link...)
 * for konqueror and kdesktop whatever the view mode is (icon, tree, ...)
 */
class KonqOperations : public QObject
{
    Q_OBJECT
protected:
    KonqOperations( QWidget * parent );
    virtual ~KonqOperations();

public:
    /**
     * Pop up properties dialog for mimetype @p mimeType.
     */
    static void editMimeType( const QString & mimeType );

    enum { TRASH, DEL, SHRED, COPY, MOVE, LINK, EMPTYTRASH, STAT };
    /**
     * @param method should be TRASH, DEL or SHRED
     */
    static void del( QWidget * parent, int method, const KURL::List & selectedURLs );

    /**
     * @param method should be COPY, MOVE or LINK
     */
    static void copy( QWidget * parent, int method, const KURL::List & selectedURLs, const KURL& destUrl );
    /**
     * Drop
     * @param destItem destination KonqFileItem for the drop (background or item)
     * @param destURL destination URL for the drop.
     * @param ev the drop event
     * @param parent parent widget (for error dialog box if any)
     *
     * If destItem is 0L, doDrop will stat the URL to determine it.
     */
    static void doDrop( const KonqFileItem * destItem, const KURL & destURL, QDropEvent * ev, QWidget * parent );

    /**
     * Paste the clipboard contents
     */
    static void doPaste( QWidget * parent, const KURL & destURL );

    static void emptyTrash();

    /**
     * Get info about a given URL, and when that's done (it's asynchronous!),
     * call a given slot with the KFileItem * as argument.
     * The KFileItem will be deleted by statURL after calling the slot. Make a copy
     * if you need one !
     */
    static void statURL( const KURL & url, const QObject *receiver, const char *member );

    static void rename( QWidget * parent, const KURL & oldurl, const QString & name );

signals:
    void statFinished( const KFileItem * item );

protected:
    enum { DEFAULT_CONFIRMATION, SKIP_CONFIRMATION, FORCE_CONFIRMATION };
    bool askDeleteConfirmation( const KURL::List & selectedURLs, int confirmation );
    void _del( int method, const KURL::List & selectedURLs, int confirmation );
    void _statURL( const KURL & url, const QObject *receiver, const char *member );

    // internal, for COPY/MOVE/LINK
    void setOperation( KIO::Job * job, int method, const KURL::List & src, const KURL & dest );

    struct DropInfo
    {
        DropInfo( uint k, KURL::List & l, int x, int y, QDropEvent::Action a ) :
            keybstate(k), lst(l), mousePos(x,y), action(a) {}
        uint keybstate;
        KURL::List lst;
        QPoint mousePos;
        QDropEvent::Action action;
    };
    // internal, for doDrop
    void setDropInfo( DropInfo * info ) { m_info = info; }

protected slots:

    void slotResult( KIO::Job * job );
    void slotStatResult( KIO::Job * job );
    void asyncDrop( const KFileItem * item );

private:
    int m_method;
    //KURL::List m_srcURLs;
    KURL m_destURL;
    // for doDrop
    DropInfo * m_info;
};

#endif
