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

#include "konq_treepart.h"
#include "dirtree_item.h"
#include "dirtree_module.h"
#include <konq_operations.h>
#include <konq_fileitem.h>
#include <konq_drag.h>
#include <ksimpleconfig.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kuserprofile.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <kio/paste.h>
#include <qfile.h>

#define MYMODULE static_cast<KonqDirTreeModule*>(module())

KonqDirTreeItem::KonqDirTreeItem( KonqTreeItem *parentItem, KonqTreeTopLevelItem *topLevelItem, KonqFileItem *fileItem )
    : KonqTreeItem( parentItem, topLevelItem ), m_fileItem( fileItem )
{
    if ( m_topLevelItem )
        MYMODULE->addSubDir( this );
    init();
}

KonqDirTreeItem::KonqDirTreeItem( KonqTree *parent, KonqTreeTopLevelItem *topLevelItem, KonqFileItem *fileItem )
    : KonqTreeItem( parent, topLevelItem ), m_fileItem( fileItem )
{
    if ( m_topLevelItem )
        MYMODULE->addSubDir( this );
    init();
}

KonqDirTreeItem::~KonqDirTreeItem()
{
}

void KonqDirTreeItem::init()
{
    // For local dirs, find out if they have no children, to remove the "+"
    if ( m_fileItem->isDir() )
    {
        KURL url = m_fileItem->url();
        if ( url.isLocalFile() )
        {
            QCString path( QFile::encodeName(url.path()));
            struct stat buff;
            if ( ::stat( path.data(), &buff ) != -1 )
            {
                //kdDebug() << "KonqDirTreeItem::init " << path << " : " << buff.st_nlink << endl;
                if ( buff.st_nlink <= 2 )
                    setExpandable( false );
            }
        }
    }
}

void KonqDirTreeItem::setOpen( bool open )
{
    kdDebug(1201) << "KonqDirTreeItem::setOpen " << open << endl;
    if ( open & !childCount() && m_bListable )
        MYMODULE->openSubFolder( this );

    KonqTreeItem::setOpen( open );
}

void KonqDirTreeItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
    if (m_fileItem->isLink())
    {
        QFont f( _painter->font() );
        f.setItalic( TRUE );
        _painter->setFont( f );
    }
    QListViewItem::paintCell( _painter, _cg, _column, _width, _alignment );
}

KURL KonqDirTreeItem::externalURL() const
{
    return m_fileItem->url();
}

QString KonqDirTreeItem::externalMimeType() const
{
    return m_fileItem->mimetype();
}

bool KonqDirTreeItem::acceptsDrops( const QStrList & formats )
{
    if ( formats.contains("text/uri-list") )
        return m_fileItem->acceptsDrops();
    return false;
}

void KonqDirTreeItem::drop( QDropEvent * ev )
{
    KonqOperations::doDrop( m_fileItem, externalURL(), ev, tree() );
}

QDragObject * KonqDirTreeItem::dragObject( QWidget * parent, bool move )
{
    KURL::List lst;
    lst.append( m_fileItem->url() );

    KonqDrag * drag = KonqDrag::newDrag( lst, false, parent );
    drag->setMoveSelection( move );

    return drag;
}

void KonqDirTreeItem::itemSelected()
{
    bool bInTrash = false;

    if ( m_fileItem->url().directory(false) == KGlobalSettings::trashPath() )
        bInTrash = true;

    QMimeSource *data = QApplication::clipboard()->data();
    bool paste = ( data->encodedData( data->format() ).size() != 0 );

    tree()->part()->extension()->enableActions( true, true, paste,
                                                true && !bInTrash, true, true );
}

void KonqDirTreeItem::middleButtonPressed()
{
    // Duplicated from KonqDirPart :(
    // Optimisation to avoid KRun to call kfmclient that then tells us
    // to open a window :-)
    KService::Ptr offer = KServiceTypeProfile::preferredService(m_fileItem->mimetype(), true);
    if (offer) kdDebug(1201) << "KonqDirPart::mmbClicked: got service " << offer->desktopEntryName() << endl;
    if ( offer && offer->desktopEntryName().startsWith("kfmclient") )
    {
        KParts::URLArgs args;
        args.serviceType = m_fileItem->mimetype();
        emit tree()->part()->extension()->createNewWindow( m_fileItem->url(), args );
    }
    else
        m_fileItem->run();
}

void KonqDirTreeItem::rightButtonPressed()
{
    KFileItemList lstItems;
    lstItems.append( m_fileItem );
    emit tree()->part()->extension()->popupMenu( QCursor::pos(), lstItems );
}

void KonqDirTreeItem::paste()
{
    // move or not move ?
    bool move = false;
    QMimeSource *data = QApplication::clipboard()->data();
    if ( data->provides( "application/x-kde-cutselection" ) ) {
        move = KonqDrag::decodeIsCutSelection( data );
        kdDebug(1201) << "move (from clipboard data) = " << move << endl;
    }

    KIO::pasteClipboard( m_fileItem->url(), move );
}

void KonqDirTreeItem::trash()
{
    delOperation( KonqOperations::TRASH );
}

void KonqDirTreeItem::del()
{
    delOperation( KonqOperations::DEL );
}

void KonqDirTreeItem::shred()
{
    delOperation( KonqOperations::SHRED );
}

void KonqDirTreeItem::delOperation( int method )
{
    KURL::List lst;
    lst.append(m_fileItem->url());

    KonqOperations::del(tree(), method, lst);
}

QString KonqDirTreeItem::toolTipText() const
{
    if ( m_fileItem->url().isLocalFile() )
	return m_fileItem->url().path();

    return m_fileItem->url().prettyURL();
}
