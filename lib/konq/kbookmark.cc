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

#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>

#include <kurl.h>
#include <kapp.h>
#include <klocale.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kstringhandler.h>
#include <kprotocolinfo.h>

#include <kmimetype.h>
#include <kio/global.h>

#include <qstringlist.h>

template class QDict<KBookmark>;
template class QList<KBookmark>;

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
    QString bookmark_path(KGlobal::dirs()->saveLocation("data", "kfm/bookmarks/Toolbar", true));
    // copy over the .directory file if it's not there
    if (!KStandardDirs::exists(bookmark_path + "/.directory"))
    {
      QCString cmd;
      cmd.sprintf( "cp %s %s/.directory",
          QFile::encodeName(locate("data", "kbookmark/directory_bookmarkbar.desktop")).data(),
          QFile::encodeName(bookmark_path).data() );
      system( cmd );
    }

    s_pSelf = new KBookmarkManager( path );
  }

  assert ( s_pSelf );

  return s_pSelf;
}

KBookmarkManager::KBookmarkManager( QString _path ) : m_sPath( _path )
{
  m_Toolbar = 0;
  m_bAllowSignalChanged = true;
  m_Root = new KBookmark( this, 0L, QString::null );
  if ( s_pSelf )
    delete s_pSelf;
  s_pSelf = this;

  m_lstParsedDirs.setAutoDelete( true );

  m_bNotify = true;

  scan( m_sPath );

  connect( KDirWatch::self(), SIGNAL( dirty( const QString & ) ),
           this, SLOT( slotNotify( const QString & ) ) );

  KDirWatch::self()->addDir( m_sPath );
}

KBookmarkManager::~KBookmarkManager()
{
  KDirWatch::self()->removeDir( m_sPath );
  delete m_Root;
  s_pSelf = 0L;
}

void KBookmarkManager::FilesAdded( const KURL & directory )
{
  if (directory.isLocalFile())
    slotNotify( directory.path() );
}

void KBookmarkManager::FilesRemoved( const KURL::List & fileList )
{
  KURL::List::ConstIterator it = fileList.begin();
  for ( ; it != fileList.end() ; ++it )
  {
    if ((*it).isLocalFile())
      slotNotify( (*it).directory() );
  }
}

void KBookmarkManager::FilesChanged( const KURL::List & fileList )
{
  KURL::List::ConstIterator it = fileList.begin();
  for ( ; it != fileList.end() ; ++it )
  {
    // TODO: make this smarter (if we show the file, reparse it and update the kbookmark)
    if ((*it).isLocalFile())
      slotNotify( (*it).directory() );
  }
}

void KBookmarkManager::slotNotify( const QString &_url )
{
  //kdDebug(1203) << "KBookmarkManager::slotNotify( " << _url << ")" << endl;
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

  if ( p1.left(p2.length()) == p2 )
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
  }
}

void KBookmarkManager::scan( const QString & _path )
{
  //kdDebug(1203) << "KBookmarkManager::scan" << endl;
  if (m_Toolbar)
    m_Toolbar->clear();

  m_Root->clear();

  // Do not emit 'changed' signals here.
  m_bAllowSignalChanged = false;
  scanIntern( m_Root, _path );
  m_lstParsedDirs.clear();
  m_bAllowSignalChanged = true;

  emitChanged();
}

void KBookmarkManager::scanIntern( KBookmark *_bm, const QString & _path )
{
  //kdDebug(1203) << "KBookmarkManager::scanIntern" << endl;
  // Substitute all symbolic links in the path
  QDir dir( _path );
  QString canonical = dir.canonicalPath();
  QString *s;
  // Did we scan this one already ?
  for( s = m_lstParsedDirs.first(); s != 0L; s = m_lstParsedDirs.next() )
  {
    if ( *s == canonical )
    {
      kdWarning() << "Directory " << s << " already parsed" << endl;
      return;
    }
  }
  m_lstParsedDirs.append( new QString( canonical ) );

  DIR *dp;
  struct dirent *ep;
  dp = opendir( QFile::encodeName(_path) );
  if ( dp == 0L )
    return;

  // Loop thru all directory entries
  while ( ( ep = readdir( dp ) ) != 0L )
  {
    if ( strcmp( ep->d_name, "." ) != 0 && strcmp( ep->d_name, ".." ) != 0 )
    {
      KURL file;
      file.setPath( QString( _path ) + '/' + ep->d_name );

      KMimeType::Ptr res = KMimeType::findByURL( file, 0, true );
      //kdDebug(1203) << " - " << file.url() << "  ->  " << res->name() << endl;

      if ( res->name() == "inode/directory" )
      {
        KBookmark* bm = new KBookmark( this, _bm, KIO::decodeFileName( ep->d_name ) );
        if ( KIO::decodeFileName( ep->d_name ) == "Toolbar" )
            m_Toolbar = bm;
        scanIntern( bm, file.path() );
      }
      else if ( res->name() == "application/x-desktop" )
      {
        KSimpleConfig cfg( file.path(), true );
        cfg.setDesktopGroup();
        QString type = cfg.readEntry( "Type" );
        // Is it really a bookmark file ?
        if ( type == "Link" )
          (void) new KBookmark( this, _bm, ep->d_name, cfg, 0 /* desktop group */ );
         else
           kdWarning(1203) << "  Not a link ? Type=" << type << endl;
      }
      else if ( res->name() == "text/plain")
      {
        // maybe its an IE Favourite..
        KSimpleConfig cfg( file.path(), true );
        QStringList grp = cfg.groupList().grep( "internetshortcut", false );
        if ( grp.count() == 0 )
            continue;

        cfg.setGroup( *grp.begin() );

        QString url = cfg.readEntry("URL");
        if (!url.isEmpty() )
          (void) new KBookmark( this, _bm, ep->d_name, cfg, *grp.begin() );
      } else kdWarning(1203) << "Invalid bookmark : found mimetype='" << res->name() << "' for file='" << file.path() << "'!" << endl;
    }
  }

  closedir( dp );
}

void KBookmarkManager::editBookmarks( const QString & _url )
{
  KRun::runURL( _url, "inode/directory" );
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
                      KSimpleConfig& _cfg, const QString &_group )
{
  assert( _bm != 0L );
  assert( _parent != 0L );

  if ( !_group.isEmpty() )
    _cfg.setGroup( _group );
  else
    _cfg.setDesktopGroup();

  m_url = _cfg.readEntry( "URL" );
  m_sPixmap = _cfg.readEntry( "Icon", QString::null );
  if (m_sPixmap.right( 4 ) == ".xpm" ) // prevent warnings
  {
    m_sPixmap.truncate( m_sPixmap.length() - 4 );
    // Should we update the config file ?
  }

  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );

  m_text = KIO::decodeFileName( _text );
  if ( m_text.length() > 8 && m_text.right( 8 ) == ".desktop" )
    m_text.truncate( m_text.length() - 8 );
  if ( m_text.length() > 7 && m_text.right( 7 ) == ".kdelnk" )
    m_text.truncate( m_text.length() - 7 );


  m_type = URL;

  m_file = _parent->file();
  m_file += '/';
  m_file += _text;

  _parent->append( _text, this );

  m_pManager->emitChanged();
}

KBookmark::KBookmark( KBookmarkManager *_bm, KBookmark *_parent, QString _text )
{
  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );
  m_type = Folder;
  m_text = _text;

  QString dir = _bm->path();
  if ( _parent )
    dir = _parent->file();
  m_file = dir;
  if ( !_text.isEmpty() )
  {
    m_file += '/';
    m_file += KIO::encodeFileName( _text );
  }

  // test for the .directory file
  QString directory_file( m_file + "/.directory" );
  if ( QFile::exists( directory_file ) )
  {
    KSimpleConfig cfg( directory_file );
    cfg.setDesktopGroup();

    m_sortOrder = cfg.readListEntry( "SortOrder" );

    //CT having all directories named "Bookmark entries" is completely useless
    QString name_text = cfg.readEntry( "Name", m_text );
    if (name_text == "Bookmark entries")
      cfg.writeEntry("Name", m_text);
    else
      m_text = name_text;
  }

  if ( _parent )
    _parent->append( _text, this );

  if ( m_pManager )
    m_pManager->emitChanged();
}

KBookmark::KBookmark( KBookmarkManager *_bm, KBookmark *_parent, QString _text, const KURL & url )
{
  assert( _bm != 0L );
  assert( _parent != 0L );
  assert( !_text.isEmpty() && !url.isEmpty() );

  QString icon;
  if ( url.isLocalFile() )
  {
    struct stat buff;
    QCString path = QFile::encodeName( url.path());
    stat( path.data(), &buff );
    icon = KMimeType::findByURL( url, buff.st_mode, true )->icon( url, true );
  }
  else
  {
    icon = KMimeType::findByURL( url )->icon( url, false );
    static const QString& unknown = KGlobal::staticQString("unknown");
    if ( icon == unknown )
        icon = KProtocolInfo::icon( url.protocol() );
  }

  m_id = g_id++;
  m_pManager = _bm;
  m_lstChildren.setAutoDelete( true );
  m_text = _text;
  m_url = url.url();
  m_type = URL;

  m_file = _parent->file();
  m_file += '/';
  m_file += KIO::encodeFileName( _text );
  m_file += ".desktop"; // We need the extension, otherwise saving a URL will
  // create a file named ".html", which will give us a wrong mimetype.

  FILE *f = fopen( QFile::encodeName(m_file), "w" );
  if ( f == 0L )
  {
    KMessageBox::sorry( 0, i18n("Could not write bookmark" ) );
    return;
  }

  fprintf( f, "[Desktop Entry]\n" );
  fprintf( f, "URL=%s\n", m_url.utf8().data() );
  fprintf( f, "Icon=%s\n", icon.latin1() );
  fprintf( f, "Type=Link\n" );
  fclose( f );

  _parent->append( _text, this );

  m_pManager->emitChanged();
}

KBookmark *KBookmark::first()
{
    m_sortIt = m_sortOrder.begin();
    if ( m_sortIt == m_sortOrder.end() )
        return 0L;
    KBookmark * valid = m_bookmarkMap.find(*m_sortIt);
    if ( valid )
        return valid;
    else
        return next(); // first one was no good, keep looking
}

KBookmark *KBookmark::next()
{
    // try to skip invalid entries in the sort list
    KBookmark *valid = 0;
    while (!valid)
    {
        m_sortIt++;
        if ( m_sortIt == m_sortOrder.end() )
            return 0L;

        valid = m_bookmarkMap.find(*m_sortIt);
    }
    return valid;
}

void KBookmark::append( const QString& _name, KBookmark *_bm )
{
  m_lstChildren.append( _bm );
  m_bookmarkMap.insert( _name, _bm );
  if ( !m_sortOrder.contains( _name ) )
    m_sortOrder.append( _name );
}

void KBookmark::clear()
{
  //kdDebug(1203) << "KBookmark::clear" << endl;
  KBookmark *bm;

  for ( bm = children()->first(); bm != 0L; bm = children()->next() )
  {
    bm->clear();
  }

  m_lstChildren.clear();
  m_bookmarkMap.clear();
}

KBookmark* KBookmark::findBookmark( int _id )
{
  if ( _id == id() )
    return this;

  KBookmark *bm;

  for ( bm = children()->first(); bm != 0L; bm = children()->next() )
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

QString KBookmark::text() const
{
  return KStringHandler::csqueeze(m_text);
}

QString KBookmark::pixmapFile()
{
  if ( m_sPixmap.isEmpty() )
  {
    QCString path = QFile::encodeName( m_file );
    struct stat buff;
    stat( path.data(), &buff );
    KURL url;
    url.setPath( m_file );
    // Get the full path to the Small icon and store it into m_sPixmap
    KMimeType::pixmapForURL( url, buff.st_mode, KIcon::Small,
            0, KIcon::DefaultState, &m_sPixmap );
  }
  return m_sPixmap;
}

void KBookmarkOwner::openBookmarkURL(const QString& url)
{
  (void) new KRun(url);
}

#include "kbookmark.moc"
