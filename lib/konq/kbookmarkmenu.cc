// -*- mode: c++; c-basic-offset: 2 -*-
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


#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "kbookmarkmenu.h"
#include "kbookmarkimporter.h"
#include "kdirnotify_stub.h"

#include <qdir.h>
#include <qfile.h>
#include <qstring.h>
#include <qpopupmenu.h>

#include <kaction.h>
#include <kapp.h>
#include <kbookmark.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kmimetype.h>
#include <kstdaction.h>
#include <kpopupmenu.h>
#include <kstdaccel.h>

template class QList<KBookmarkMenu>;

/********************************************************************
 *
 * KBookmarkMenu
 *
 ********************************************************************/

KBookmarkMenu::KBookmarkMenu( KBookmarkOwner * _owner, QPopupMenu * _parentMenu,
                              KActionCollection *collec, bool _isRoot, bool _add,
                              const QString & parentAddress )
  : m_bIsRoot(_isRoot), m_bAddBookmark(_add), m_pOwner(_owner),
    m_parentMenu( _parentMenu ),
    m_actionCollection( collec ),
    m_parentAddress( parentAddress )
{
  m_lstSubMenus.setAutoDelete( true );
  m_actions.setAutoDelete( true );

  m_bNSBookmark = m_parentAddress.isNull();
  if ( !m_bNSBookmark ) // not for the netscape bookmark
  {
    //kdDebug(1203) << "KBookmarkMenu::KBookmarkMenu " << this << " address : " << m_parentAddress << endl;

    connect( _parentMenu, SIGNAL( aboutToShow() ),
             SLOT( slotAboutToShow() ) );

    if ( m_bIsRoot )
    {
      connect( KBookmarkManager::self(), SIGNAL( changed(const QString &, const QString &) ),
               SLOT( slotBookmarksChanged(const QString &) ) );
    }
  }

  // add entries that possibly have a shortcut, so they are available _before_ first popup
  if ( m_bIsRoot )
  {
    if ( m_bAddBookmark )
      addAddBookmark();

    addEditBookmarks();
  }

  m_bDirty = true;
}

KBookmarkMenu::~KBookmarkMenu()
{
  QListIterator<KAction> it( m_actions );
  for (; it.current(); ++it )
    it.current()->unplugAll();

  m_lstSubMenus.clear();
  m_actions.clear();
}

void KBookmarkMenu::ensureUpToDate()
{
  slotAboutToShow();
}


void KBookmarkMenu::slotAboutToShow()
{
  // Did the bookmarks change since the last time we showed them ?
  if ( m_bDirty )
  {
    m_bDirty = false;
    refill();
  }
}

void KBookmarkMenu::slotBookmarksChanged( const QString & groupAddress )
{
  if (m_bNSBookmark)
    return;

  if ( groupAddress == m_parentAddress )
  {
    //kdDebug(1203) << "KBookmarkMenu::slotBookmarksChanged -> setting m_bDirty on " << groupAddress << endl;
    m_bDirty = true;
  }
  else
  {
    // Iterate recursively into child menus
    QListIterator<KBookmarkMenu> it( m_lstSubMenus );
    for (; it.current(); ++it )
    {
      it.current()->slotBookmarksChanged( groupAddress );
    }
  }
}

void KBookmarkMenu::refill()
{
  //kdDebug(1203) << "KBookmarkMenu::refill()" << endl;
  m_lstSubMenus.clear();

  QListIterator<KAction> it( m_actions );
  for (; it.current(); ++it )
    it.current()->unplug( m_parentMenu );

  m_parentMenu->clear();
  m_actions.clear();

  fillBookmarkMenu();
  m_parentMenu->adjustSize();
}

void KBookmarkMenu::addAddBookmark()
{
  // create "add bookmark", with the parent's ID (as a name)
  KAction * paAddBookmarks = new KAction( i18n( "&Add Bookmark" ),
                                          "bookmark_add",
                                          m_bIsRoot ? KStdAccel::addBookmark() : 0,
                                          this,
                                          SLOT( slotAddBookmark() ),
                                          m_actionCollection );

  paAddBookmarks->setStatusText( i18n( "Add a bookmark for the current document" ) );

  paAddBookmarks->plug( m_parentMenu );
  m_actions.append( paAddBookmarks );
}

void KBookmarkMenu::addEditBookmarks()
{
  KAction * m_paEditBookmarks = KStdAction::editBookmarks( KBookmarkManager::self(), SLOT( slotEditBookmarks() ),
                                                             m_actionCollection, "edit_bookmarks" );
  m_paEditBookmarks->plug( m_parentMenu );
  m_paEditBookmarks->setStatusText( i18n( "Edit your bookmark collection in a separate window" ) );
  m_actions.append( m_paEditBookmarks );
}

void KBookmarkMenu::addNewFolder()
{
  KAction * paNewFolder = new KAction( i18n( "&New Folder..." ),
                                       "folder_new", //"folder",
                                       0,
                                       this,
                                       SLOT( slotNewFolder() ),
                                       m_actionCollection );

  paNewFolder->setStatusText( i18n( "Create a new bookmark folder in this menu" ) );

  paNewFolder->plug( m_parentMenu );
  m_actions.append( paNewFolder );
}

void KBookmarkMenu::fillBookmarkMenu()
{
  if ( m_bIsRoot )
  {
    if ( m_bAddBookmark )
      addAddBookmark();

    addEditBookmarks();

    if ( m_bAddBookmark )
      addNewFolder();

    m_parentMenu->insertSeparator();

    if ( KBookmarkManager::self()->showNSBookmarks()
         && QFile::exists( KNSBookmarkImporter::netscapeBookmarksFile() ) )
    {
      KActionMenu * actionMenu = new KActionMenu( i18n("Netscape Bookmarks"), "netscape",
                                                  m_actionCollection, 0L );
      actionMenu->plug( m_parentMenu );
      m_actions.append( actionMenu );
      KBookmarkMenu *subMenu = new KBookmarkMenu( m_pOwner, actionMenu->popupMenu(),
                                                  m_actionCollection, false,
                                                  m_bAddBookmark, QString::null );
      m_lstSubMenus.append(subMenu);
      connect(actionMenu->popupMenu(), SIGNAL(aboutToShow()), subMenu, SLOT(slotNSLoad()));
      m_parentMenu->insertSeparator();
    }
  }

  KBookmarkGroup parentBookmark = KBookmarkManager::self()->findByAddress( m_parentAddress ).toGroup();
  ASSERT(!parentBookmark.isNull());
  for ( KBookmark bm = parentBookmark.first(); !bm.isNull();  bm = parentBookmark.next(bm) )
  {
    if ( !bm.isGroup() )
    {
      if ( bm.isSeparator() )
      {
        m_parentMenu->insertSeparator();
      }
      else
      {
        //kdDebug(1203) << "Creating URL bookmark menu item for " << bm.text() << endl;
        // create a normal URL item, with ID as a name
        QString text = bm.text();
        text.replace( QRegExp( "&" ), "&&" );
        KAction * action = new KAction( text, bm.icon(), 0,
                                        this, SLOT( slotBookmarkSelected() ),
                                        m_actionCollection, bm.url().url().utf8() );

        action->setStatusText( bm.url().prettyURL() );

        action->plug( m_parentMenu );
        m_actions.append( action );
      }
    }
    else
    {
      //kdDebug(1203) << "Creating bookmark submenu named " << bm.text() << endl;
      KActionMenu * actionMenu = new KActionMenu( bm.text(), bm.icon(),
                                                  m_actionCollection, 0L );
      actionMenu->plug( m_parentMenu );
      m_actions.append( actionMenu );
      KBookmarkMenu *subMenu = new KBookmarkMenu( m_pOwner, actionMenu->popupMenu(),
                                                  m_actionCollection, false,
                                                  m_bAddBookmark,
                                                  bm.address() );
      m_lstSubMenus.append( subMenu );
    }
  }

  if ( !m_bIsRoot && m_bAddBookmark )
  {
    m_parentMenu->insertSeparator();
    addAddBookmark();
    addNewFolder();
  }
}

void KBookmarkMenu::slotAddBookmark()
{
  QString url = m_pOwner->currentURL();
  if (url.isEmpty())
  {
    KMessageBox::error( 0L, i18n("Can't add bookmark with empty url"));
    return;
  }
  QString title = m_pOwner->currentTitle();
  if (title.isEmpty())
    title = url;

  KBookmarkGroup parentBookmark = KBookmarkManager::self()->findByAddress( m_parentAddress ).toGroup();
  ASSERT(!parentBookmark.isNull());
  // If this title is already used, we'll try to find something unused.
  KBookmark ch = parentBookmark.first();
  int count = 1;
  QString uniqueTitle = title;
  do
  {
    while ( !ch.isNull() )
    {
      if ( uniqueTitle == ch.text() )
      {
        // Title already used !
        if ( url != ch.url().url() )
        {
          uniqueTitle = title + QString(" (%1)").arg(++count);
          // New title -> restart search from the beginning
          ch = parentBookmark.first();
          break;
        }
        else
        {
          // this exact URL already exists
          return;
        }
      }
      ch = parentBookmark.next( ch );
    }
  } while ( !ch.isNull() );

  parentBookmark.addBookmark( uniqueTitle, url );
  KBookmarkManager::self()->emitChanged( parentBookmark );
}

void KBookmarkMenu::slotNewFolder()
{
  if ( !m_pOwner ) return; // this view doesn't handle bookmarks...
  KBookmarkGroup parentBookmark = KBookmarkManager::self()->findByAddress( m_parentAddress ).toGroup();
  ASSERT(!parentBookmark.isNull());
  KBookmarkGroup group = parentBookmark.createNewFolder();
  if ( !group.isNull() )
  {
    KBookmarkGroup parentGroup = group.parentGroup();
    KBookmarkManager::self()->emitChanged( parentGroup );
  }
}

void KBookmarkMenu::slotBookmarkSelected()
{
  //kdDebug(1203) << "KBookmarkMenu::slotBookmarkSelected()" << endl;
  if ( !m_pOwner ) return; // this view doesn't handle bookmarks...
  //kdDebug(1203) << sender()->name() << endl;

  // The name of the action is the URL to open
  m_pOwner->openBookmarkURL( QString::fromUtf8(sender()->name()) );
}

// -----------------------------------------------------------------------------

void KBookmarkMenu::slotNSBookmarkSelected()
{
    QString link(sender()->name()+8);

    m_pOwner->openBookmarkURL( link );
}

void KBookmarkMenu::slotNSLoad()
{
  m_parentMenu->disconnect(SIGNAL(aboutToShow()));

  KBookmarkMenuNSImporter importer( this, m_actionCollection );
  importer.openNSBookmarks();
}

void KBookmarkMenuNSImporter::openNSBookmarks()
{
  mstack.push(m_menu);
  KNSBookmarkImporter importer( KNSBookmarkImporter::netscapeBookmarksFile() );
  connect( &importer, SIGNAL( newBookmark( const QString &, const QCString &, const QString & ) ),
           SLOT( newBookmark( const QString &, const QCString &, const QString & ) ) );
  connect( &importer, SIGNAL( newFolder( const QString &, bool, const QString & ) ),
           SLOT( newFolder( const QString &, bool, const QString & ) ) );
  connect( &importer, SIGNAL( newSeparator() ), SLOT( newSeparator() ) );
  connect( &importer, SIGNAL( endMenu() ), SLOT( endMenu() ) );
  importer.parseNSBookmarks();
}

void KBookmarkMenuNSImporter::newBookmark( const QString & text, const QCString & url, const QString & )
{
  QCString actionLink = "bookmark" + url;
  KAction * action = new KAction( text, "html", 0, m_menu, SLOT( slotNSBookmarkSelected() ),
                                  m_actionCollection, actionLink.data());
  action->setStatusText( url );
  action->plug( mstack.top()->m_parentMenu );
  mstack.top()->m_actions.append( action );
}

void KBookmarkMenuNSImporter::newFolder( const QString & text, bool, const QString & )
{
  KActionMenu * actionMenu = new KActionMenu( text, "folder", m_actionCollection, 0L );
  actionMenu->plug( mstack.top()->m_parentMenu );
  mstack.top()->m_actions.append( actionMenu );
  KBookmarkMenu *subMenu = new KBookmarkMenu( m_menu->m_pOwner, actionMenu->popupMenu(),
                                              m_actionCollection, false,
                                              m_menu->m_bAddBookmark, QString::null );
  mstack.top()->m_lstSubMenus.append( subMenu );

  mstack.push(subMenu);
}

void KBookmarkMenuNSImporter::newSeparator()
{
  mstack.top()->m_parentMenu->insertSeparator();
}

void KBookmarkMenuNSImporter::endMenu()
{
  mstack.pop();
}

#include "kbookmarkmenu.moc"
