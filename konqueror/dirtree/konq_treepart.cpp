/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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
#include "konq_tree.h"
#include <kparts/factory.h>
#include <kinstance.h>
#include <kdebug.h>
#include <konq_drag.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qdir.h>

KInstance *KonqTreeFactory::s_instance = 0;

KonqTreeFactory::~KonqTreeFactory()
{
    if ( s_instance )
    {
        delete s_instance;
        s_instance = 0L;
    }
}

KParts::Part* KonqTreeFactory::createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList & )
{
    KParts::Part *obj = new KonqTreePart( parentWidget, parent, name );
    emit objectCreated( obj );
    return obj;
}

KInstance *KonqTreeFactory::instance()
{
    if ( !s_instance )
        s_instance = new KInstance( "konqueror" );
    return s_instance;
}

extern "C"
{
  void *init_libkonqtree()
  {
    return new KonqTreeFactory;
  }
};

/////////////////

KonqTreeBrowserExtension::KonqTreeBrowserExtension( KonqTreePart *parent, KonqTree *tree )
 : KParts::BrowserExtension( parent )
{
    m_pTree = tree;
}

void KonqTreeBrowserExtension::enableActions( bool copy, bool cut, bool paste,
                                              bool trash, bool del, bool shred,
                                              bool rename )
{
    emit enableAction( "copy", copy );
    emit enableAction( "cut", cut );
    emit enableAction( "paste", paste );
    emit enableAction( "trash", trash );
    emit enableAction( "del", del );
    emit enableAction( "shred", shred );
    emit enableAction( "rename", rename );
}

void KonqTreeBrowserExtension::cut()
{
    QDragObject * drag = static_cast<KonqTreeItem*>(m_pTree->selectedItem())->dragObject( 0L, true );
    if (drag)
        QApplication::clipboard()->setData( drag );
}

void KonqTreeBrowserExtension::copy()
{
    QDragObject * drag = static_cast<KonqTreeItem*>(m_pTree->selectedItem())->dragObject( 0L );
    if (drag)
        QApplication::clipboard()->setData( drag );
}

void KonqTreeBrowserExtension::paste()
{
    if (m_pTree->currentItem())
        m_pTree->currentItem()->paste();
}

void KonqTreeBrowserExtension::trash()
{
    if (m_pTree->currentItem())
        m_pTree->currentItem()->trash();
}

void KonqTreeBrowserExtension::del()
{
    if (m_pTree->currentItem())
        m_pTree->currentItem()->del();
}

void KonqTreeBrowserExtension::shred()
{
    if (m_pTree->currentItem())
        m_pTree->currentItem()->shred();
}

void KonqTreeBrowserExtension::rename()
{
    if (m_pTree->currentItem())
        m_pTree->currentItem()->rename();
}

////////////////////

KonqTreePart::KonqTreePart( QWidget *parentWidget, QObject *parent, const char *name )
 : KParts::ReadOnlyPart( parent, name )
{
    m_pTree = new KonqTree( this, parentWidget );
    m_extension = new KonqTreeBrowserExtension( this, m_pTree );

    setWidget( m_pTree );
    setInstance( KonqTreeFactory::instance(), false );
    m_url.setPath( QDir::homeDirPath() );
}

KonqTreePart::~KonqTreePart()
{
}

bool KonqTreePart::openURL( const KURL & url )
{
  // We need to update m_url so that view-follows-view works
  // (we need other views to follow us, so we need a proper m_url)
  m_url = url;

  emit started( 0 );
  m_pTree->followURL( url );
  emit completed();
  return true;
}

void KonqTreePart::emitStatusBarText( const QString& text )
{
    emit setStatusBarText( text );
}

#include "konq_treepart.moc"
