/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include <qdir.h>

#include "kbookmarkmenu.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

#include <kurl.h>
#include <kbookmark.h>
#include <kapp.h>
#include <kwm.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <qstring.h>

#include "kmimetypes.h"
#include "kpixmapcache.h"

#include <opUIUtils.h>
#include <klocale.h>

/********************************************************************
 *
 * KBookmarkMenu
 *
 ********************************************************************/

KBookmarkMenu::KBookmarkMenu( KBookmarkOwner * _owner, OpenPartsUI::Menu_ptr menu, OpenParts::Part_ptr part, bool _root )
  : m_bIsRoot(_root), m_pOwner(_owner)
{
  m_lstSubMenus.setAutoDelete( true );

  assert( !CORBA::is_nil( menu ) );

  m_vMenu = OpenPartsUI::Menu::_duplicate( menu );
  m_vPart = OpenParts::Part::_duplicate( part );
  m_vMenu->connect( "activated", m_vPart, "slotBookmarkSelected" );
  m_vMenu->connect( "highlighted", m_vPart, "slotBookmarkHighlighted" );

  if ( m_bIsRoot )
  {
    connect( KBookmarkManager::self(), SIGNAL( changed() ), this, SLOT( slotBookmarksChanged() ) );
    slotBookmarksChanged();
  }
}

KBookmarkMenu::~KBookmarkMenu()
{
  m_lstSubMenus.clear();

  assert( !CORBA::is_nil( m_vMenu ) );

  m_vMenu->disconnect( "activated", m_vPart, "slotBookmarkSelected" );
  m_vMenu->disconnect( "highlighted", m_vPart, "slotBookmarkHighlighted" );

  m_vMenu = 0L;
}

void KBookmarkMenu::slotBookmarksChanged()
{
  assert( !CORBA::is_nil( m_vMenu ) );

  m_lstSubMenus.clear();

  if ( !m_bIsRoot )
  {
    m_vMenu->disconnect( "activated", m_vPart, "slotBookmarkSelected" );
    m_vMenu->disconnect( "highlighted", m_vPart, "slotBookmarkHighlighted" );
  }    

  m_vMenu->clear();

  if ( m_bIsRoot )
  {
    QString text = i18n("&Edit Bookmarks...");
    m_vMenu->insertItem( text, m_vPart, "slotEditBookmarks", 0 );
  }    

  kdebug(0, 1203, "fillBookmarkMenu : starting (this his hopefully faster than before)");
  fillBookmarkMenu( KBookmarkManager::self()->root() );
  kdebug(0, 1203, "fillBookmarkMenu : done");
}

void KBookmarkMenu::fillBookmarkMenu( KBookmark *parent )
{
  KBookmark *bm;
  QString text;

  assert( !CORBA::is_nil( m_vMenu ) );
  // kdebug(0, 1202, "KBookmarkMenu::fillBookmarkMenu( %p )", parent);

  text = i18n("&Add Bookmark");
  m_vMenu->insertItem7( text, (CORBA::Long)parent->id(), -1 );
  m_vMenu->insertSeparator( -1 );

  // Create one OpenPartsUI::Pixmap (will be reused for each call)
  OpenPartsUI::Pixmap* pix = new OpenPartsUI::Pixmap;
  pix->onlyFilename = true;

  for ( bm = parent->children()->first(); bm != 0L;  bm = parent->children()->next() )
  {
    QString pixmapFullPath = KPixmapCache::pixmapFile( bm->pixmapFile(), true /* mini icon */ );
    pix->data = pixmapFullPath.data();
    text = bm->text();
    if ( bm->type() == KBookmark::URL )
    {
        m_vMenu->insertItem11( *pix, text, (CORBA::Long)bm->id(), -1 );	
    }
    else
    {	
        OpenPartsUI::Menu_var subMenuVar;
        m_vMenu->insertItem12( *pix, text, subMenuVar, -1, -1 );
        KBookmarkMenu *subMenu = new KBookmarkMenu( m_pOwner, subMenuVar, m_vPart, false );
        m_lstSubMenus.append( subMenu );
        subMenu->fillBookmarkMenu( bm );
    }
  }
  delete pix;
}

void KBookmarkMenu::slotBookmarkSelected( int _id )
{
  if ( !m_pOwner ) return; // this view doesn't handle bookmarks...
  
  KBookmark *bm = KBookmarkManager::self()->findBookmark( _id );

  if ( bm )
  {
    if ( bm->type() == KBookmark::Folder )
    {
      QString title = m_pOwner->currentTitle();
      QString url = m_pOwner->currentURL();
      (void)new KBookmark( KBookmarkManager::self(), bm, title, url );
      return;
    }

    KURL u( bm->url() );
    if ( u.isMalformed() )
    {
      QString tmp = i18n("Malformed URL\n%1").arg(bm->url());
      KMessageBox::error( 0L, tmp);
      return;
    }
	
    m_pOwner->openBookmarkURL( bm->url() );
  }
}

#include "kbookmarkmenu.moc"
