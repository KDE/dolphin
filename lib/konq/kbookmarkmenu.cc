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
#include "kdirnotify_stub.h"

#include <qdir.h>
#include <qfile.h>
#include <qstack.h>
#include <qstring.h>
#include <qpopupmenu.h>

#include <kaction.h>
#include <kapp.h>
#include <kbookmark.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <kglobal.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kmimetype.h>
#include <kstringhandler.h>
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
                              KActionCollection * _collec, bool _root, bool _add,
                              KBookmark * _parentBookmark )
  : m_bIsRoot(_root), m_bAddBookmark(_add), m_pOwner(_owner),
    m_parentMenu( _parentMenu ), m_actionCollection( _collec ),
    m_parentBookmark( _parentBookmark ), m_mtime((time_t)0)
{
  m_lstSubMenus.setAutoDelete( true );
  m_actions.setAutoDelete( true );

  if ( m_bIsRoot )
    {
      connect( KBookmarkManager::self(), SIGNAL( changed() ), this,
	       SLOT( slotBookmarksChanged() ) );
      slotBookmarksChanged();
    }
  if ( m_parentBookmark ) // not for the netscape bookmark
    connect( _parentMenu, SIGNAL( aboutToShow() ),
             SLOT( slotAboutToShow() ) );
}

KBookmarkMenu::~KBookmarkMenu()
{
  QListIterator<KAction> it( m_actions );
  for (; it.current(); ++it )
    it.current()->unplugAll();

  m_lstSubMenus.clear();
}

void KBookmarkMenu::slotAboutToShow()
{
  struct stat statbuf;
  kdDebug(1203) << "KBookmarkMenu::slotAboutToShow " << m_parentBookmark->file() << endl;
  if ( stat( QFile::encodeName(m_parentBookmark->file()), &statbuf ) == 0 )
  {
    if ( statbuf.st_mtime > m_mtime )
    {
      m_mtime = statbuf.st_mtime;
      kdDebug() << "Updating mtime" << endl;
      slotBookmarksChanged();
    }
  }
}

void KBookmarkMenu::slotBookmarksChanged()
{
  kdDebug(1203) << "KBookmarkMenu::slotBookmarksChanged()" << endl;
  m_lstSubMenus.clear();

  QListIterator<KAction> it( m_actions );
  for (; it.current(); ++it )
    it.current()->unplug( m_parentMenu );

  m_parentMenu->clear();
  m_actions.clear();

  fillBookmarkMenu();
}

void KBookmarkMenu::fillBookmarkMenu()
{
  kdDebug() << "KBookmarkMenu::fillBookmarkMenu " << m_parentBookmark->file() << endl;
  if ( m_bAddBookmark )
  {
    // create the first item, add bookmark, with the parent's ID (as a name)
    KAction * paAddBookmarks = new KAction( i18n( "&Add Bookmark" ),
                                              "bookmark_add",
                                              m_bIsRoot ? KStdAccel::addBookmark() : 0,
                                              this,
                                              SLOT( slotBookmarkSelected() ),
                                              m_actionCollection,
                                              QCString().sprintf("bookmark%d", m_parentBookmark->id()) );

    paAddBookmarks->setStatusText( i18n( "Add a bookmark for the current document" ) );

    paAddBookmarks->plug( m_parentMenu );
    m_actions.append( paAddBookmarks );
  }

  if ( m_bIsRoot )
  {
    KAction * m_paEditBookmarks = KStdAction::editBookmarks( KBookmarkManager::self(), SLOT( slotEditBookmarks() ), m_actionCollection, "edit_bookmarks" );
    m_paEditBookmarks->plug( m_parentMenu );
    m_paEditBookmarks->setStatusText( i18n( "Edit your bookmark collection in a separate window" ) );
    m_actions.append( m_paEditBookmarks );

    if ( !m_bAddBookmark )
      m_parentMenu->insertSeparator();
  }

  if ( m_bAddBookmark )
  {
    KAction * paNewFolder = new KAction( i18n( "&New Folder..." ),
                                         "folder_new", //"folder",
                                              0,
                                              this,
                                              SLOT( slotBookmarkSelected() ),
                                              m_actionCollection,
                                              QCString().sprintf("newfolder%d", m_parentBookmark->id()) );

    paNewFolder->setStatusText( i18n( "Create a new bookmark folder in this menu" ) );

    paNewFolder->plug( m_parentMenu );
    m_actions.append( paNewFolder );

    m_parentMenu->insertSeparator();
  }

  if ( m_bIsRoot )
    {
      KActionMenu * actionMenu = new KActionMenu( i18n("Netscape Bookmarks"), "netscape",
                                                  m_actionCollection, 0L );
      actionMenu->plug( m_parentMenu );
      m_actions.append( actionMenu );
      KBookmarkMenu *subMenu = new KBookmarkMenu( m_pOwner, actionMenu->popupMenu(),
                                                  m_actionCollection, false,
                                                  m_bAddBookmark, 0 );
      m_lstSubMenus.append(subMenu);
      connect(actionMenu->popupMenu(), SIGNAL(aboutToShow()), subMenu, SLOT(slotNSLoad()));
      m_parentMenu->insertSeparator();
    }

  for ( KBookmark * bm = m_parentBookmark->first(); bm != 0L;  bm = m_parentBookmark->next() )
  {
    if ( bm->type() == KBookmark::URL )
    {
      //kdDebug(1203) << "Creating URL bookmark menu item for " << bm->text() << endl;
      // create a normal URL item, with ID as a name
      KAction * action = new KAction( bm->text(), bm->pixmapFile(), 0,
                                      this, SLOT( slotBookmarkSelected() ),
                                      m_actionCollection, QCString().sprintf("bookmark%d", bm->id()) );

      action->setStatusText( bm->url() );

      action->plug( m_parentMenu );
      m_actions.append( action );
    }
    else
    {
      KActionMenu * actionMenu = new KActionMenu( bm->text(), bm->pixmapFile(),
                                                  m_actionCollection, 0L );
      actionMenu->plug( m_parentMenu );
      m_actions.append( actionMenu );
      KBookmarkMenu *subMenu = new KBookmarkMenu( m_pOwner, actionMenu->popupMenu(),
                                                  m_actionCollection, false,
                                                  m_bAddBookmark, bm );
      m_lstSubMenus.append( subMenu );
      // let's delay this. subMenu->fillBookmarkMenu( bm );
    }
  }
}

void KBookmarkMenu::slotNSBookmarkSelected()
{
    QString link(sender()->name()+8);

    m_pOwner->openBookmarkURL( link );
}

void KBookmarkMenu::slotNSLoad()
{
  m_parentMenu->disconnect(SIGNAL(aboutToShow()));
  openNSBookmarks();
}

void KBookmarkMenu::slotBookmarkSelected()
{
  //kdDebug(1203) << "KBookmarkMenu::slotBookmarkSelected()" << endl;
  if ( !m_pOwner ) return; // this view doesn't handle bookmarks...
  kdDebug(1203) << sender()->name() << endl;

  if (QCString(sender()->name()).left(9) == "newfolder" )
  {
    int id = QString( sender()->name() + 9 ).toInt(); // skip "newfolder"
    KBookmark *bm = KBookmarkManager::self()->findBookmark( id );
    if ( bm )
    {
      KLineEditDlg l( i18n("New Folder:"), "", 0L );
      l.setCaption( i18n("Create new folder in %1").arg(bm->file()) );
      if ( l.exec() )
      {
        KURL url;
        url.setPath( bm->file() + "/" + KIO::encodeFileName(l.text()) );
        KIO::Job * job = KIO::mkdir( url );
        connect( job, SIGNAL( result( KIO::Job * ) ),
                 SLOT( slotResult( KIO::Job * ) ) );
      }
    }
    else
      kdError(1203) << "Bookmark " << id << " not found !" << endl;
  }
  else
  {
    int id = QString( sender()->name() + 8 ).toInt(); // skip "bookmark"
    KBookmark *bm = KBookmarkManager::self()->findBookmark( id );

    if ( bm )
    {
      if ( bm->type() == KBookmark::Folder ) // "Add bookmark"
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

        // If this title is already used, we'll try to find something
        // unused.
        KBookmark *ch = bm->first();
        int count = 1;
        QString uniqueTitle = title;
        do
        {
          while ( ch )
          {
            if ( uniqueTitle == ch->text() )
            {
              if ( url != ch->url() )
              {
                uniqueTitle = title + QString(" (%1)").arg(++count);
                ch = bm->first();
                break;
              }
              else
              {
                // this exact URL already exists
                return;
              }
            }
            ch = bm->next();
          }
        } while ( ch );

        (void)new KBookmark( KBookmarkManager::self(), bm, uniqueTitle, url );
        // DCOP broadcast to notify about this new bookmark
        KURL uChanged;
        uChanged.setPath( bm->file() );
        KDirNotify_stub allDirNotify("*", "KDirNotify*");
        allDirNotify.FilesAdded( uChanged );
        kdDebug(1203) << "KDirNotify notified for " << uChanged.url() << endl;
        return;
      }

      KURL u( bm->url() );
      if ( u.isMalformed() || u.isEmpty() )
      {
        QString tmp = i18n("Malformed URL\n%1").arg(bm->url());
        KMessageBox::error( 0L, tmp);
        return;
      }

      m_pOwner->openBookmarkURL( bm->url() );
    }
    else
      kdError(1203) << "Bookmark not found !" << endl;
  }
}

void KBookmarkMenu::slotResult( KIO::Job * job )
{
  if (job->error())
    job->showErrorDialog();
}

// -----------------------------------------------------------------------------

void KBookmarkMenu::openNSBookmarks()
{
  QFile f(QDir::homeDirPath() + "/.netscape/bookmarks.html");
  QStack<KBookmarkMenu> mstack;
  mstack.push(this);
  QRegExp amp("&amp;");
  QRegExp lt("&lt;");
  QRegExp gt("&gt;");

  if(f.open(IO_ReadOnly)) {

    QCString s(1024);
    // skip header
    while(f.readLine(s.data(), 1024) >= 0 && !s.contains("<DL>"));

    while(f.readLine(s.data(), 1024)>=0) {
      QCString t = s.stripWhiteSpace();
      if(t.left(12) == "<DT><A HREF=" ||
         t.left(16) == "<DT><H3><A HREF=") {
        int firstQuotes = t.find('"')+1;
        QCString link = t.mid(firstQuotes, t.find('"', firstQuotes)-firstQuotes);
        QCString name = t.mid(t.find('>', 15)+1);
        QCString actionLink = "bookmark" + link;

        name = name.left(name.findRev('<'));
        if ( name.right(4) == "</A>" )
           name = name.left( name.length() - 4 );
        name.replace( amp, "&" ).replace( lt, "<" ).replace( gt, ">" );

        KAction * action = new KAction( KStringHandler::csqueeze(QString::fromLocal8Bit(name)), "html", 0,
                                        this, SLOT( slotNSBookmarkSelected() ),
                                        m_actionCollection, actionLink.data());
        action->setStatusText( link );
        action->plug( mstack.top()->m_parentMenu );
        mstack.top()->m_actions.append( action );
      }
      else if(t.left(7) == "<DT><H3") {
        QCString name = t.mid(t.find('>', 7)+1);
        name = name.left(name.findRev('<'));
        name.replace( amp, "&" ).replace( lt, "<" ).replace( gt, ">" );

        KActionMenu * actionMenu = new KActionMenu( KStringHandler::csqueeze(QString::fromLocal8Bit(name)), "folder",
                                                    m_actionCollection, 0L );
        actionMenu->plug( mstack.top()->m_parentMenu );
        mstack.top()->m_actions.append( actionMenu );
        KBookmarkMenu *subMenu = new KBookmarkMenu( m_pOwner, actionMenu->popupMenu(),
                                                    m_actionCollection, false,
                                                    m_bAddBookmark, 0 );
        mstack.top()->m_lstSubMenus.append( subMenu );

        mstack.push(subMenu);
      }
      else if(t.left(4) == "<HR>")
        mstack.top()->m_parentMenu->insertSeparator();
      else if(t.left(8) == "</DL><p>")
        mstack.pop();
    }

    f.close();
  }
}


#include "kbookmarkmenu.moc"
