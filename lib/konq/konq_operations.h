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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __konq_operations_h__
#define __konq_operations_h__

#include <kurl.h>
#include <libkonq_export.h>

#include <QObject>
#include <QEvent>
//Added by qt3to4:
#include <QList>
#include <QDropEvent>

class KJob;
namespace KIO { class Job; class CopyInfo; }
class QWidget;
class KFileItem;
class KonqMainWindow;

/**
 * Implements file operations (move,del,trash,shred,paste,copy,move,link...)
 * for konqueror and kdesktop whatever the view mode is (icon, tree, ...)
 */
class LIBKONQ_EXPORT KonqOperations : public QObject
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

    enum { TRASH, DEL, SHRED, COPY, MOVE, LINK, EMPTYTRASH, STAT, MKDIR, RESTORE, UNKNOWN };
    /**
     * Delete the @p selectedURLs if possible.
     *
     * @param parent parent widget (for error dialog box if any)
     * @param method should be TRASH, DEL or SHRED
     * @param selectedURLs the URLs to be deleted
     */
    static void del( QWidget * parent, int method, const KUrl::List & selectedURLs );

    /**
     * Copy the @p selectedURLs to the destination @p destURL.
     *
     * @param parent parent widget (for error dialog box if any)
     * @param method should be COPY, MOVE or LINK
     * @param selectedURLs the URLs to copy
     * @param destURL destination of the copy
     *
     * @todo document restrictions on the kind of destination
     */
    static void copy( QWidget * parent, int method, const KUrl::List & selectedURLs, const KUrl& destURL );
    /**
     * Drop
     * @param destItem destination KFileItem for the drop (background or item)
     * @param destURL destination URL for the drop.
     * @param ev the drop event
     * @param parent parent widget (for error dialog box if any)
     *
     * If destItem is 0L, doDrop will stat the URL to determine it.
     */
    static void doDrop( const KFileItem * destItem, const KUrl & destURL, QDropEvent * ev, QWidget * parent );

    /**
     * Paste the clipboard contents
     */
    static void doPaste( QWidget * parent, const KUrl & destURL, const QPoint &pos );
    static void doPaste( QWidget * parent, const KUrl & destURL );

    static void emptyTrash();
    static void restoreTrashedItems( const KUrl::List& urls );

    /**
     * Create a directory
     */
    static void mkdir( QWidget *parent, const KUrl & url );

    /**
     * Get info about a given URL, and when that's done (it's asynchronous!),
     * call a given slot with the KFileItem * as argument.
     * The KFileItem will be deleted by statURL after calling the slot. Make a copy
     * if you need one !
     */
    static void statURL( const KUrl & url, const QObject *receiver, const char *member );

    /**
     * Do a renaming.
     * @param parent the parent widget, passed to KonqOperations ctor
     * @param oldurl the current url of the file to be renamed
     * @param name the new name for the file. Shouldn't include '/'.
     */
    static void rename( QWidget * parent, const KUrl & oldurl, const QString & name );

    /**
     * Do a renaming.
     * @param parent the parent widget, passed to KonqOperations ctor
     * @param oldurl the current url of the file to be renamed
     * @param newurl the new url for the file
     * Use this version if the other one wouldn't work :)  (e.g. because name could
     * be a relative path, including a '/').
     */
    static void rename( QWidget * parent, const KUrl & oldurl, const KUrl & newurl );

    /**
     * Ask for the name of a new directory and create it.
     * @param parent the parent widget
     * @param baseURL the directory to create the new directory in
     */
    static void newDir( QWidget * parent, const KUrl & baseURL );

Q_SIGNALS:
    void statFinished( const KFileItem * item );
    void aboutToCreate(const QPoint &pos, const QList<KIO::CopyInfo> &files);

protected:
    enum { DEFAULT_CONFIRMATION, SKIP_CONFIRMATION, FORCE_CONFIRMATION };
    bool askDeleteConfirmation( const KUrl::List & selectedURLs, int confirmation );
    void _del( int method, const KUrl::List & selectedURLs, int confirmation );
    void _restoreTrashedItems( const KUrl::List& urls );
    void _statURL( const KUrl & url, const QObject *receiver, const char *member );

    // internal, for COPY/MOVE/LINK/MKDIR
    void setOperation( KIO::Job * job, int method, const KUrl::List & src, const KUrl & dest );

    struct DropInfo
    {
        DropInfo( uint k, const KUrl::List & l, const QMap<QString,QString> &m,
                  int x, int y, Qt::DropAction a ) :
            keybstate(k), lst(l), metaData(m), mousePos(x,y), action(a) {}
        uint keybstate;
        KUrl::List lst;
        QMap<QString,QString> metaData;
        QPoint mousePos;
        Qt::DropAction action;
    };
    // internal, for doDrop
    void setDropInfo( DropInfo * info ) { m_info = info; }

    struct KIOPasteInfo // KDE4: remove and use DropInfo instead or a QPoint member
    {
        QByteArray data;  // unused
        KUrl destURL;     // unused
        QPoint mousePos;
        QString dialogText; // unused
    };
    void setPasteInfo( KIOPasteInfo * info ) { m_pasteInfo = info; }

protected Q_SLOTS:

    void slotAboutToCreate(KIO::Job *job, const QList<KIO::CopyInfo> &files);
    void slotResult( KJob * job );
    void slotStatResult( KJob * job );
    void asyncDrop( const KFileItem * item );
    void doFileCopy();

private:
    int m_method;
    //KUrl::List m_srcURLs;
    KUrl m_destURL;
    // for doDrop
    DropInfo * m_info;
    KIOPasteInfo * m_pasteInfo;
};

#include <kio/job.h>

/// Restore multiple trashed files
class KonqMultiRestoreJob : public KIO::Job
{
    Q_OBJECT

public:
    KonqMultiRestoreJob( const KUrl::List& urls, bool showProgressInfo );

protected Q_SLOTS:
    virtual void slotStart();
    virtual void slotResult( KJob *job );

private:
    const KUrl::List m_urls;
    KUrl::List::const_iterator m_urlsIterator;
    int m_progress;
};

#endif
