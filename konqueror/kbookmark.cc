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

#include "kbookmark.h"

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

/**
 * Gloabl ID for bookmarks.
 */
int g_id = 0;

/********************************************************************
 *
 * KBookmarkManager
 *
 ********************************************************************/

KBookmarkManager* KBookmarkManager::s_pSelf = 0L;

KBookmarkManager* KBookmarkManager::self()
{
  assert ( s_pSelf );

  return s_pSelf;
}

KBookmarkManager::KBookmarkManager() : m_Root( 0L, 0L, 0L )
{
  s_pSelf = this;

  m_lstParsedDirs.setAutoDelete( true );

  // Little hack
  m_Root.m_pManager = this;
  m_bAllowSignalChanged = true;
  m_bNotify = true;

  QString p = kapp->localkdedir();
  p += "/share/apps/kfm/bookmarks";
  scan( p );

  // HACK
  // connect( KIOServer::getKIOServer(), SIGNAL( notify( const char* ) ),
  // this, SLOT( slotNotify( const char* ) ) );
}

void KBookmarkManager::slotNotify( const char *_url )
{
  if ( !m_bNotify )
    return;

  KURL u( _url );
  if ( !u.isLocalFile() )
    return;

  QString p = kapp->localkdedir();
  p += "/share/apps/kfm/bookmarks";
  QDir dir2( p );
  QDir dir1( u.path() );

  QString p1( dir1.canonicalPath() );
  QString p2( dir2.canonicalPath() );
  if ( p1.isEmpty() )
    p1 = u.path();
  if ( p2.isEmpty() )
    p2 = p;

  if ( qstrncmp( p1, p2, p2.length() ) == 0 )
  {
    QString d = kapp->localkdedir();
    d += "/share/apps/kfm/bookmarks/";
    scan( d );
  }
}

void KBookmarkManager::emitChanged()
{
  // Scanning right now ?
  if ( m_bAllowSignalChanged )
  {
    // ... no => emit signal
    emit changed();
    // tell krootwm to refresh the bookmarks popup menu
    KWM::sendKWMCommand ("krootwm:refreshBM");
  }
}

void KBookmarkManager::scan( const char * _path )
{
  m_Root.clear();

  // Do not emit 'changed' signals here.
  m_bAllowSignalChanged = false;
  scanIntern( &m_Root, _path );
  m_lstParsedDirs.clear();
  m_bAllowSignalChanged = true;

  emitChanged();
}

void KBookmarkManager::scanIntern( KBookmark *_bm, const char * _path )
{
  // Substitute all symbolic links in the path
  QDir dir( _path );
  QString canonical = dir.canonicalPath();
  QString *s;
  // Did we scan this one already ?
  for( s = m_lstParsedDirs.first(); s != 0L; s = m_lstParsedDirs.next() )
  {
    if ( qstrcmp( *s, canonical ) == 0 )
      return;
  }
  m_lstParsedDirs.append( new QString( canonical ) );

  DIR *dp;
  struct dirent *ep;
  dp = opendir( _path );
  if ( dp == 0L )
    return;

  // Loop thru all directory entries
  while ( ( ep = readdir( dp ) ) != 0L )
  {
    if ( strcmp( ep->d_name, "." ) != 0 && strcmp( ep->d_name, ".." ) != 0 )
    {
      // QString name = ep->d_name;	

      QString file = _path;
      file += "/";
      file += ep->d_name;
      struct stat buff;
      stat( file.data(), &buff );

      if ( S_ISDIR( buff.st_mode ) )
      {
	KBookmark* bm = new KBookmark( this, _bm, KBookmark::decode( ep->d_name ) );
	scanIntern( bm, file );
      }
      else if ( S_ISREG( buff.st_mode ) )
      {
	// Is it really a kde config file ?
	bool ok = true;
	
	FILE *f;
	f = fopen( file, "rb" );
	if ( f == 0L )
	  ok = false;
	else
	{
	  char buff[ 100 ];
	  buff[ 0 ] = 0;
	  fgets( buff, 100, f );
	  fclose( f );
	
	  if ( strncmp( buff, "# KDE Config File", 17 ) != 0L )
	    ok = false;
	}
	
	if ( ok )
	{
	  KSimpleConfig cfg( file, true );
	  cfg.setGroup( "KDE Desktop Entry" );
	  QString type = cfg.readEntry( "Type" );	
	  // Is it really a bookmark file ?
	  if ( type == "Link" )
	    (void) new KBookmark( this, _bm, ep->d_name, cfg );
	}
      }
    }
  }

  closedir( dp );
}

void KBookmarkManager::slotEditBookmarks()
{
  QString q = kapp->localkdedir();
  KURL u ( q + "/share/apps/kfm/bookmarks");

  editBookmarks( u.url() );
}

/********************************************************************
 *
 * KBookmark
 *
 ********************************************************************/

KBookmark::KBookmark( KBookmarkManager *_bm, KBookmark *_parent, const char *_text, KSimpleConfig& _cfg )
{
  assert( _bm != 0L );
  assert( _parent != 0L );

  _cfg.setGroup( "KDE Desktop Entry" );
  m_url = _cfg.readEntry( "URL" );

  m_pPixmap = 0L;
  m_pMiniPixmap = 0L;
  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );

  m_text = KBookmark::decode( _text );
  if ( m_text.length() > 7 && m_text.right( 7 ) == ".kdelnk" )
    m_text.truncate( m_text.length() - 7 );

  m_type = URL;

  m_file = _parent->file();
  m_file += "/";
  m_file += _text;

  _parent->append( this );

  m_pManager->emitChanged();
}

KBookmark::KBookmark( KBookmarkManager *_bm, KBookmark *_parent, const char *_text )
{
  m_pPixmap = 0L;
  m_pMiniPixmap = 0L;
  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );
  m_type = Folder;
  m_text = _text;

  QString p = kapp->localkdedir();
  p += "/share/apps/kfm/bookmarks";
  const char *dir = p;
  if ( _parent )
    dir = _parent->file();
  m_file = dir;
  if ( _text )
  {
    m_file += "/";
    m_file += encode( _text );
  }

  if ( _parent )
    _parent->append( this );

  if ( m_pManager )
    m_pManager->emitChanged();
}

KBookmark::KBookmark( KBookmarkManager *_bm, KBookmark *_parent, const char *_text, const char *_url )
{
  assert( _bm != 0L );
  assert( _parent != 0L );
  assert( _text != 0L && _url != 0L );

  KURL u( _url );

  string icon;
  if ( u.isLocalFile() )
  {
    struct stat buff;
    stat( u.path(), &buff );
    icon = KMimeType::findByURL( u, buff.st_mode, true )->icon( u.path(), true );
  }
  else if ( strcmp( u.protocol(), "ftp" ) == 0 )
    icon = "ftp.xpm";
  else
    icon = "www.xpm";

  m_pPixmap = 0L;
  m_pMiniPixmap = 0L;
  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );
  m_text = _text;
  m_url = _url;
  m_type = URL;

  m_file = _parent->file();
  m_file += "/";
  m_file += encode( _text );
  // m_file += ".kdelnk";

  FILE *f = fopen( m_file, "w" );
  if ( f == 0L )
  {
    QMessageBox::critical( (QWidget*)0L, i18n(" Error"), i18n("Could not write bookmark" ) );
    return;
  }

  fprintf( f, "# KDE Config File\n" );
  fprintf( f, "[KDE Desktop Entry]\n" );
  fprintf( f, "URL=%s\n", m_url.data() );
  fprintf( f, "Icon=%s\n", icon.c_str() );
  fprintf( f, "MiniIcon=%s\n", icon.c_str() );
  fprintf( f, "Type=Link\n" );
  fclose( f );

  m_pManager->disableNotify();

  // Update opened KFM windows. Perhaps there is one
  // that shows the bookmarks directory.
  QString fe( _parent->file() );
  // To make an URL, we have th encode the path
  KURL::encode( fe );
  fe.prepend( "file:" );
  // HACK
  // KIOServer::sendNotify( fe );

  m_pManager->enableNotify();

  _parent->append( this );

  m_pManager->emitChanged();
}

void KBookmark::clear()
{
  KBookmark *bm;

  for ( bm = children()->first(); bm != NULL; bm = children()->next() )
  {
    bm->clear();
  }

  m_lstChildren.clear();
}

KBookmark* KBookmark::findBookmark( int _id )
{
  if ( _id == id() )
    return this;

  KBookmark *bm;

  for ( bm = children()->first(); bm != NULL; bm = children()->next() )
  {
    if ( bm->id() == _id )
      return bm;

    if ( bm->type() == Folder )
    {
      KBookmark *b = bm->findBookmark( _id );
      if ( b )
	return b;
    }
  }

  return 0L;
}

QString KBookmark::encode( const char *_str )
{
  QString str( _str );

/* FIXME: temporarily disabled... I'm too lazy to fix this now ;) (Simon)
  int i = 0;
  while ( ( i = str.find( "%", i ) ) != -1 )
  {
    str.replace( i, 1, "%%");
    i += 2;
  }
  while ( ( i = str.find( "/" ) ) != -1 )
      str.replace( i, 1, "%2f");
*/
  return str;
}

QString KBookmark::decode( const char *_str )
{
  QString str( _str );

/* FIXME: temporarily disabled... I'm too lazy to fix this now ;) (Simon)
  
  int i = 0;
  while ( ( i = str.find( "%%", i ) ) != -1 )
  {
    str.replace( i, 2, "%");
    i++;
  }

  while ( ( i = str.find( "%2f" ) ) != -1 )
      str.replace( i, 3, "/");
  while ( ( i = str.find( "%2F" ) ) != -1 )
      str.replace( i, 3, "/");
*/
  return str;
}

QPixmap* KBookmark::pixmap( bool _mini )
{
  if ( m_pPixmap == 0L )
  {
    struct stat buff;
    stat( m_file, &buff );
    QString encoded = m_file.data();
    KURL::encode( encoded );
    m_pPixmap = KPixmapCache::pixmapForURL( encoded, buff.st_mode, true, _mini );
  }

  assert( m_pPixmap );

  return m_pPixmap;
}

/********************************************************************
 *
 * KBookmarkMenu
 *
 ********************************************************************/

KBookmarkMenu::KBookmarkMenu( KBookmarkOwner *_owner, OpenPartsUI::Menu_ptr menu, OpenParts::Part_ptr part, bool _root )
{
  m_pOwner = _owner;
  m_bIsRoot = _root;

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
      KBookmarkMenu *subMenu = new KBookmarkMenu( m_pOwner, subMenuVar, m_vPart, false );
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
	
    m_pOwner->openBookmarkURL( bm->url() );
  }
}

#include "kbookmark.moc"
