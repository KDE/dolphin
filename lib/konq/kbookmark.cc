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
#include <klocale.h>
#include <kwm.h>
#include <kmimemagic.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <krun.h>

#include <kmimetype.h>
#include "kfileitem.h"

/**
 * Global ID for bookmarks.
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
  if ( !s_pSelf )
  {
    QString path(KGlobal::dirs()->saveLocation("data", "kfm/bookmarks", true));
    s_pSelf = new KBookmarkManager( path );
  }

  assert ( s_pSelf );

  return s_pSelf;
}

KBookmarkManager::KBookmarkManager( QString _path ) : m_sPath( _path )
{
  m_Root = new KBookmark( this, 0L, QString::null );
  if ( s_pSelf )
    delete s_pSelf;
  s_pSelf = this;

  m_lstParsedDirs.setAutoDelete( true );

  m_bAllowSignalChanged = true;
  m_bNotify = true;

  scan( m_sPath );

  connect( KDirWatch::self(), SIGNAL( dirty( const QString & ) ),
           this, SLOT( slotNotify( const QString & ) ) );
}

KBookmarkManager::~KBookmarkManager()
{
  KDirWatch::self()->removeDir( m_sPath );
  delete m_Root;
}

void KBookmarkManager::slotNotify( const QString &_url )
{
  if ( !m_bNotify )
    return;

  KURL u( _url );
  if ( !u.isLocalFile() )
    return;

  QDir dir2( m_sPath );
  QDir dir1( u.path() );

  QString p1( dir1.canonicalPath() );
  QString p2( dir2.canonicalPath() );
  if ( p1.isEmpty() )
    p1 = u.path();
  if ( p2.isEmpty() )
    p2 = m_sPath;

  if ( qstrncmp( p1, p2, p2.length() ) == 0 )
  {
    scan( m_sPath );
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
  m_Root->clear();

  // Do not emit 'changed' signals here.
  m_bAllowSignalChanged = false;
  scanIntern( m_Root, _path );
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
      QString file = _path;
      file += '/';
      file += ep->d_name;

      KMimeMagicResult * res = KMimeMagic::self()->findFileType( file );

      if ( res->mimeType() == "inode/directory" )
      {
        KBookmark* bm = new KBookmark( this, _bm, KFileItem::decodeFileName( ep->d_name ) );
        scanIntern( bm, file );
      }
      else if ( res->mimeType() == "application/x-desktop" )
      {
        KSimpleConfig cfg( file, true );
        cfg.setDesktopGroup();
        QString type = cfg.readEntry( "Type" );	
        // Is it really a bookmark file ?
        if ( type == "Link" )
          (void) new KBookmark( this, _bm, ep->d_name, cfg, QString::null /* desktop group */ );
      }
      else if ( res->mimeType() == "text/plain")
      {
        // maybe its an IE Favourite..
        KSimpleConfig cfg( file, true );
        cfg.setGroup("InternetShortCut");
        QString url = cfg.readEntry("URL");
        if (!url.isEmpty() )
          (void) new KBookmark( this, _bm, ep->d_name, cfg, "InternetShortCut" );
      } else kdebug(KDEBUG_WARN, 1203, "Invalid bookmark : found mimetype='%s' for file='%s'!",
                    res->mimeType().ascii(), file.ascii());
    }
  }

  closedir( dp );
}

void KBookmarkManager::editBookmarks( const char *_url )
{
  KFileManager::getFileManager()->openFileManagerWindow( _url );
}

void KBookmarkManager::slotEditBookmarks()
{
  editBookmarks( m_sPath );
}

/********************************************************************
 *
 * KBookmark
 *
 ********************************************************************/

KBookmark::KBookmark( KBookmarkManager *_bm, KBookmark *_parent, QString _text,
                      KSimpleConfig& _cfg, const char * _group )
{
  assert( _bm != 0L );
  assert( _parent != 0L );

  if ( _group )
    _cfg.setGroup( _group );
  else
    _cfg.setDesktopGroup();

  m_url = _cfg.readEntry( "URL", "ERROR ! No URL !" );
  m_sPixmap = _cfg.readEntry( "Icon", QString::null );
  if (m_sPixmap.right( 4 ) == ".xpm" ) // prevent warnings
  {
    m_sPixmap.truncate( m_sPixmap.length() - 4 );
    // Should we update the config file ?
  }

  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );

  m_text = KFileItem::decodeFileName( _text );
  if ( m_text.length() > 8 && m_text.right( 8 ) == ".desktop" )
    m_text.truncate( m_text.length() - 8 );
  if ( m_text.length() > 7 && m_text.right( 7 ) == ".kdelnk" )
    m_text.truncate( m_text.length() - 7 );


  m_type = URL;

  m_file = _parent->file();
  m_file += '/';
  m_file += _text;

  _parent->append( this );

  m_pManager->emitChanged();
}

KBookmark::KBookmark( KBookmarkManager *_bm, KBookmark *_parent, QString _text )
{
  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );
  m_type = Folder;
  m_text = _text;

  const char *dir = _bm->path();
  if ( _parent )
    dir = _parent->file();
  m_file = dir;
  if ( !_text.isEmpty() )
  {
    m_file += '/';
    m_file += KFileItem::encodeFileName( _text );
  }

  // test for the .directory file
  QString directory_file( m_file + "/.directory" );
  if ( QFile::exists( directory_file ) )
  {
    KSimpleConfig cfg( directory_file, true );
    cfg.setDesktopGroup();

    //CT having all directories named "Bookmark entries" is completely useless
    QString name_text = cfg.readEntry( "Name", m_text );
    if (name_text == "Bookmark entries")
      cfg.writeEntry("Name", m_text);
    else
      m_text = name_text;
  }

  if ( _parent )
    _parent->append( this );

  if ( m_pManager )
    m_pManager->emitChanged();
}

KBookmark::KBookmark( KBookmarkManager *_bm, KBookmark *_parent, QString _text, QString _url )
{
  assert( _bm != 0L );
  assert( _parent != 0L );
  assert( !_text.isEmpty() && !_url.isEmpty() );

  KURL u( _url );

  QString icon;
  if ( u.isLocalFile() )
  {
    struct stat buff;
    stat( u.path(), &buff );
    icon = KMimeType::findByURL( u, buff.st_mode, true )->icon( u.path(), true );
  }
  else if ( strcmp( u.protocol(), "ftp" ) == 0 )
    icon = "ftp";
  else
    icon = "www";

  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );
  m_text = _text;
  m_url = _url;
  m_type = URL;

  m_file = _parent->file();
  m_file += '/';
  m_file += KFileItem::encodeFileName( _text );
  // m_file += ".desktop"; // looks better to the user without extension

  FILE *f = fopen( m_file, "w" );
  if ( f == 0L )
  {
    KMessageBox::sorry( 0, i18n("Could not write bookmark" ) );
    return;
  }

  //fprintf( f, "# KDE Config File\n" );
  fprintf( f, "[Desktop Entry]\n" );
  fprintf( f, "URL=%s\n", m_url.ascii() );
  fprintf( f, "Icon=%s\n", icon.ascii() );
  //fprintf( f, "MiniIcon=%s\n", icon.ascii() );
  fprintf( f, "Type=Link\n" );
  fclose( f );

  m_pManager->disableNotify();

  // Update opened KFM windows. Perhaps there is one
  // that shows the bookmarks directory.
  //QString fe( _parent->file() );
  // To make an URL, we have th encode the path
  //KURL::encode( fe );
  //fe.prepend( "file:" );
  // HACK
  // KIOServer::sendNotify( fe );

  m_pManager->enableNotify();

  _parent->append( this );

  m_pManager->emitChanged();
}

void KBookmark::append( KBookmark *_bm )
{ 
  /*CT: Kurt said this will be configurable
  if ( _bm->type() == Folder && BookmarkFoldersFirst)  
      lstChildren.prepend( _bm) : m_lstChildren.append( _bm );
  */
  /* David : Yeah, what about appending the item nonetheless ? */
  m_lstChildren.append( _bm );
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

QString KBookmark::pixmapFile( )
{
  if ( m_sPixmap.isEmpty() )
  {
    struct stat buff;
    stat( m_file, &buff );
    QString url = m_file;
    KURL::encode( url );
    // Get the full path to the Small icon and store it into m_sPixmap
    KMimeType::pixmapForURL( KURL( url ), buff.st_mode, KIconLoader::Small, &m_sPixmap );
  }
  return m_sPixmap;
}

//static
QString KBookmark::stringSqueeze( const QString & str, unsigned int maxlen )
{
  if (str.length() > maxlen) {
    int part = (maxlen-3)/2;
    return QString(str.left(part) + "..." + str.right(part));
  }
  else return str;
}

#include "kbookmark.moc"
