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

#include <qstring.h>
#include <qpopupmenu.h>

#include <kaction.h>
#include <kapp.h>
#include <kbookmark.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kwm.h>
#include <kstdaction.h>

#include <kmimetype.h>

template class QList<KBookmarkMenu>;

/********************************************************************
 *
 * KBookmarkMenu
 *
 ********************************************************************/

KBookmarkMenu::KBookmarkMenu( KBookmarkOwner * _owner, QPopupMenu * _parentMenu, QActionCollection * _collec, bool _root, bool _add )
  : m_bIsRoot(_root), m_bAddBookmark(_add), m_pOwner(_owner), m_parentMenu( _parentMenu ), m_actionCollection( _collec )
{
  m_lstSubMenus.setAutoDelete( true );

  if ( m_bIsRoot )
  {
    connect( KBookmarkManager::self(), SIGNAL( changed() ), this, SLOT( slotBookmarksChanged() ) );
    slotBookmarksChanged();
  }
}

KBookmarkMenu::~KBookmarkMenu()
{
  m_lstSubMenus.clear();
}

void KBookmarkMenu::slotBookmarksChanged()
{
  m_lstSubMenus.clear();

  if ( !m_bIsRoot )
  {
    //  m_vMenu->disconnect( "highlighted", m_vPart, "slotBookmarkHighlighted" );
  }

  m_parentMenu->clear();

  if ( m_bIsRoot )
  {
    KAction * m_paEditBookmarks = KStdAction::editBookmarks( KBookmarkManager::self(), SLOT( slotEditBookmarks() ), m_actionCollection, "edit_bookmarks" );
    m_paEditBookmarks->plug( m_parentMenu );

    if ( !m_bAddBookmark )
      m_parentMenu->insertSeparator();
  }

  KGlobal::iconLoader()->setIconType( "icon" );
  fillBookmarkMenu( KBookmarkManager::self()->root() );
  KGlobal::iconLoader()->setIconType( "toolbar" ); // restore default
}

void KBookmarkMenu::fillBookmarkMenu( KBookmark *parent )
{
  if ( m_bAddBookmark )
  {
    // create the first item, add bookmark, with the parent's ID (as a name)
    KAction * m_paAddBookmarks = new KAction( i18n( "&Add Bookmark" ),
                                              m_bIsRoot ? CTRL+Key_B : 0,
                                              this,
                                              SLOT( slotBookmarkSelected() ),
                                              m_actionCollection,
                                              QString("bookmark%1").arg(parent->id()) );
    m_paAddBookmarks->plug( m_parentMenu );
    m_parentMenu->insertSeparator();
  }

  for ( KBookmark * bm = parent->first(); bm != 0L;  bm = parent->next() )
  {
    if ( bm->type() == KBookmark::URL )
    {
      // create a normal URL item, with ID as a name
      QPixmap pix = KGlobal::iconLoader()->loadIcon(bm->pixmapFile(),
                                                               KIconLoader::Small);
      KAction * action = new KAction( bm->text(), QIconSet( pix ), 0,
                                      this, SLOT( slotBookmarkSelected() ),
                                      m_actionCollection, QString("bookmark%1").arg(bm->id()) );
      action->plug( m_parentMenu );
    }
    else
    {	
      KActionMenu * actionMenu = new KActionMenu( bm->text(), QIconSet( BarIcon( bm->pixmapFile() ) ),
                                                  m_actionCollection, 0L );
      actionMenu->plug( m_parentMenu );
      KBookmarkMenu *subMenu = new KBookmarkMenu( m_pOwner, actionMenu->popupMenu(),
                                                  m_actionCollection, false,
                                                  m_bAddBookmark );
      m_lstSubMenus.append( subMenu );
      subMenu->fillBookmarkMenu( bm );
    }
  }
}

void KBookmarkMenu::slotBookmarkSelected()
{
  debug("KBookmarkMenu::slotBookmarkSelected()");
  if ( !m_pOwner ) return; // this view doesn't handle bookmarks...
  debug( sender()->name() );

  int id = QString( sender()->name() + 8 ).toInt(); // skip "bookmark"
  KBookmark *bm = KBookmarkManager::self()->findBookmark( id );

  if ( bm )
  {
    if ( bm->type() == KBookmark::Folder ) // "Add bookmark"
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
  else
    debug("Bookmark not found !");
}

#include "kbookmarkmenu.moc"
