/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef konq_tree_h
#define konq_tree_h

#include <klistview.h>
#include "konq_treetoplevelitem.h"
#include <kdirnotify.h>
#include <qmap.h>
#include <qstrlist.h>
#include <qpixmap.h>
class KonqTreeModule;
class KonqTreeItem;
class KonqTreePart;
class QTimer;

/**
 * The multi-purpose tree (listview)
 * It parses its configuration (desktop files), each one corresponding to
 * a toplevel item, and creates the modules that will handle the contents
 * of those items.
 */
class KonqTree : public KListView, public KDirNotify
{
    Q_OBJECT
public:
    KonqTree( KonqTreePart *parent, QWidget *parentWidget );
    virtual ~KonqTree();

    void followURL( const KURL &url );

    /**
     * This returns the module associated with the current (i.e. selected) item
     */
    KonqTreeModule * currentModule() const;

    void startAnimation( KonqTreeItem * item, const char * iconBaseName );
    void stopAnimation( KonqTreeItem * item );

    // Reimplemented from KDirNotify
    void FilesAdded( const KURL & dir );
    void FilesRemoved( const KURL::List & urls );
    void FilesChanged( const KURL::List & urls );

    KonqTreePart * part() { return m_part; }

protected:
    virtual void contentsDragEnterEvent( QDragEnterEvent *e );
    virtual void contentsDragMoveEvent( QDragMoveEvent *e );
    virtual void contentsDragLeaveEvent( QDragLeaveEvent *e );
    virtual void contentsDropEvent( QDropEvent *ev );

    virtual void contentsMousePressEvent( QMouseEvent *e );
    virtual void contentsMouseMoveEvent( QMouseEvent *e );
    virtual void contentsMouseReleaseEvent( QMouseEvent *e );

private slots:
    void slotDoubleClicked( QListViewItem *item );
    void slotClicked( QListViewItem *item );
    void slotMouseButtonPressed(int _button, QListViewItem* _item, const QPoint&, int col);
    void slotSelectionChanged();

    void slotAnimation();

    void slotAutoOpenFolder();

    void rescanConfiguration();

private:
    void clearTree();
    void scanDir( KonqTreeItem *parent, const QString &path, bool isRoot = false );
    void scanDir2( KonqTreeItem *parent, const QString &path );
    void loadTopLevelItem( KonqTreeItem *parent, const QString &filename );

    QList<KonqTreeTopLevelItem> m_topLevelItems;

    QList<KonqTreeModule> m_lstModules;

    KonqTreePart *m_part;

#if 0
    typedef QMap<KonqTreeItem *, QString> MapCurrentOpeningFolders;
    MapCurrentOpeningFolders m_mapCurrentOpeningFolders;
#endif

    QTimer *m_animationTimer;

    int m_animationCounter;

    QPoint m_dragPos;
    bool m_bDrag;

    QListViewItem *m_dropItem;
    QStrList m_lstDropFormats;

    QPixmap m_folderPixmap;

    QTimer *m_autoOpenTimer;

    // The base URL for our configuration directory
    KURL m_dirtreeDir;
};

#endif
