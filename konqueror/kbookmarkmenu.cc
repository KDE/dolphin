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
#include <kapp.h>
#include <kwm.h>
#include <qmsgbox.h>

#include "kmimetypes.h"
#include "kpixmapcache.h"

#include <opUIUtils.h>

/********************************************************************
 *
 * KBookmarkMenu
 *
 ********************************************************************/

KBookmarkMenu::KBookmarkMenu( OpenPartsUI::Menu_ptr menu, OpenParts::Part_ptr part, bool _root )
  : m_pOwner(0L), m_bIsRoot(_root)
{
  m_lstSubMenus.setAutoDelete( true );

  assert( !CORBA::is_nil( menu ) );

  m_vMenu = OpenPartsUI::Menu::_duplicate( menu );
  m_vPart = OpenParts::Part::_duplicate( part );
  m_vMenu->connect( "activated", m_vPart, "slotBookmarkSelected" );

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

  m_vMenu = 0L;
}

void KBookmarkMenu::changeOwner( KBookmarkOwner *_owner )
{
  m_pOwner = _owner;
}

void KBookmarkMenu::slotBookmarksChanged()
{
  assert( !CORBA::is_nil( m_vMenu ) );

  m_lstSubMenus.clear();

  if ( !m_bIsRoot )
    m_vMenu->disconnect( "activated", m_vPart, "slotBookmarkSelected" );

  m_vMenu->clear();

  if ( m_bIsRoot )
    m_vMenu->insertItem( i18n("&Edit Bookmarks..."), m_vPart, "slotEditBookmarks", 0 );

  fillBookmarkMenu( KBookmarkManager::self()->root() );
}

void KBookmarkMenu::fillBookmarkMenu( KBookmark *parent )
{
  KBookmark *bm;

  assert( !CORBA::is_nil( m_vMenu ) );

  m_vMenu->insertItem7( i18n("&Add Bookmark"), (CORBA::Long)parent->id(), -1 );
  m_vMenu->insertSeparator( -1 );

  for ( bm = parent->children()->first(); bm != NULL;  bm = parent->children()->next() )
  {
    if ( bm->type() == KBookmark::URL )
    {
      OpenPartsUI::Pixmap_var pix = OPUIUtils::convertPixmap( *(bm->pixmap( true )) );
      m_vMenu->insertItem11( pix, bm->text(), (CORBA::Long)bm->id(), -1 );	
    }
    else
    {	
      OpenPartsUI::Menu_var subMenuVar;
      OpenPartsUI::Pixmap_var pix = OPUIUtils::convertPixmap( *(bm->pixmap( true )) );
      m_vMenu->insertItem12( pix, bm->text(), subMenuVar, -1, -1 );
      KBookmarkMenu *subMenu = new KBookmarkMenu( subMenuVar, m_vPart, false );
      m_lstSubMenus.append( subMenu );
      subMenu->fillBookmarkMenu( bm );
    }
  }
}

void KBookmarkMenu::slotBookmarkSelected( int _id )
{
  KBookmark *bm = KBookmarkManager::self()->findBookmark( _id );

  if ( bm )
  {
    if ( bm->type() == KBookmark::Folder )
    {
      assert( m_pOwner );
      QString title = m_pOwner->currentTitle();
      QString url = m_pOwner->currentURL();
      (void)new KBookmark( KBookmarkManager::self(), bm, title, url );
      return;
    }

    KURL u( bm->url() );
    if ( u.isMalformed() )
    {
      string tmp = i18n( "Malformed URL" ).ascii();
      tmp += "\n";
      tmp += bm->url();
      QMessageBox::critical( 0L, i18n( "Error" ), tmp.c_str(), i18n( "OK" ) );
      return;
    }
	
    assert( m_pOwner );
    m_pOwner->openBookmarkURL( bm->url() );
  }
}

#include "kbookmarkmenu.moc"
