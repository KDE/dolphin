/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_propsview.h"
#include "konq_treeviewitems.h"
#include "konq_treeviewwidget.h"
#include <kfileitem.h>
#include <kio_job.h>
#include <klocale.h>
#include <assert.h>

/**************************************************************
 *
 * KonqTreeViewItem
 *
 **************************************************************/

KonqTreeViewItem::KonqTreeViewItem( KonqTreeViewWidget *_treeview, KonqTreeViewDir * _parent, KFileItem* _fileitem )
  : QListViewItem( _parent ), m_fileitem( _fileitem )
{
  m_pTreeView = _treeview;
  init();
}

KonqTreeViewItem::KonqTreeViewItem( KonqTreeViewWidget *_parent, KFileItem* _fileitem )
  : QListViewItem( _parent ), m_fileitem( _fileitem )
{
  m_pTreeView = _parent;
  init();
}

void KonqTreeViewItem::init()
{
  setPixmap( 0, m_fileitem->pixmap( KIconLoader::Small, false /*no image preview*/ ) );
}

QString KonqTreeViewItem::key( int _column, bool ) const
{
  static char buffer[ 12 ];

  assert( _column < (int)m_fileitem->entry().count() );

  unsigned long uds = m_fileitem->entry()[ _column ].m_uds;

  if ( uds & KIO::UDS_STRING )
    return m_fileitem->entry()[ _column ].m_str;
  else if ( uds & KIO::UDS_LONG )
  {
    sprintf( buffer, "%08d", (int)m_fileitem->entry()[ _column ].m_long );
    return buffer;
  }
  else
    assert( 0 );
}

QString KonqTreeViewItem::text( int _column ) const
{
  //  assert( _column < (int)m_fileitem->entry().count() );

  if ( _column >= (int)m_fileitem->entry().count() )
    return "";

  const KUDSAtom & atom = m_fileitem->entry()[ _column ];
  unsigned long uds = atom.m_uds;

  if ( uds == KIO::UDS_ACCESS )
    return makeAccessString( atom );
  else if ( uds == KIO::UDS_FILE_TYPE )
    return makeTypeString( atom );
  else if ( ( uds & KIO::UDS_TIME ) == KIO::UDS_TIME )
    return makeTimeString( atom );
  else if ( uds & KIO::UDS_NAME )
    return m_fileitem->text();
  else if ( uds & KIO::UDS_STRING )
    return atom.m_str;
  else if ( uds & KIO::UDS_LONG )
    return makeNumericString( atom );
  else
    assert( 0 );
}

const char* KonqTreeViewItem::makeNumericString( const KUDSAtom &_atom ) const
{
  static char buffer[100];
  QDateTime dt; dt.setTime_t((uint) _atom.m_long);
  snprintf( buffer, 100, KGlobal::locale()->formatNumber((int) _atom.m_long, 0));
  return buffer;
}

const char* KonqTreeViewItem::makeTimeString( const KUDSAtom &_atom ) const
{
  static char buffer[100];
  QDateTime dt; dt.setTime_t((time_t) _atom.m_long);

  snprintf( buffer, 100, KGlobal::locale()->formatDateTime(dt) );

  return buffer;
}

QString KonqTreeViewItem::makeTypeString( const KUDSAtom &_atom ) const
{
  mode_t mode = (mode_t) _atom.m_long;

  if ( S_ISLNK( mode ) )
    return i18n( "Link" );
  else if ( S_ISDIR( mode ) )
    return i18n( "Directory" );
  // TODO check for devices here, too
  else
    return i18n( "File" );
}

const char* KonqTreeViewItem::makeAccessString( const KUDSAtom &_atom ) const
{
  static char buffer[ 12 ];

  mode_t mode = (mode_t) _atom.m_long;
	
  char uxbit,gxbit,oxbit;

  if ( (mode & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) )
    uxbit = 's';
  else if ( (mode & (S_IXUSR|S_ISUID)) == S_ISUID )
    uxbit = 'S';
  else if ( (mode & (S_IXUSR|S_ISUID)) == S_IXUSR )
    uxbit = 'x';
  else
    uxbit = '-';
	
  if ( (mode & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) )
    gxbit = 's';
  else if ( (mode & (S_IXGRP|S_ISGID)) == S_ISGID )
    gxbit = 'S';
  else if ( (mode & (S_IXGRP|S_ISGID)) == S_IXGRP )
    gxbit = 'x';
  else
    gxbit = '-';
	
  if ( (mode & (S_IXOTH|S_ISVTX)) == (S_IXOTH|S_ISVTX) )
    oxbit = 't';
  else if ( (mode & (S_IXOTH|S_ISVTX)) == S_ISVTX )
    oxbit = 'T';
  else if ( (mode & (S_IXOTH|S_ISVTX)) == S_IXOTH )
    oxbit = 'x';
  else
    oxbit = '-';

  buffer[0] = ((( mode & S_IRUSR ) == S_IRUSR ) ? 'r' : '-' );
  buffer[1] = ((( mode & S_IWUSR ) == S_IWUSR ) ? 'w' : '-' );
  buffer[2] = uxbit;
  buffer[3] = ((( mode & S_IRGRP ) == S_IRGRP ) ? 'r' : '-' );
  buffer[4] = ((( mode & S_IWGRP ) == S_IWGRP ) ? 'w' : '-' );
  buffer[5] = gxbit;
  buffer[6] = ((( mode & S_IROTH ) == S_IROTH ) ? 'r' : '-' );
  buffer[7] = ((( mode & S_IWOTH ) == S_IWOTH ) ? 'w' : '-' );
  buffer[8] = oxbit;
  buffer[9] = 0;

  return buffer;
}

void KonqTreeViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
  // Underline link ?
  if ( m_pTreeView->m_bSingleClick &&
       m_pTreeView->m_bUnderlineLink && _column == 0)
  {
    QFont f = _painter->font();
    f.setUnderline( true );
    _painter->setFont( f );
  }
  // TODO text color
  
  if (!m_pTreeView->props()->bgPixmap().isNull())
  {
    _painter->drawTiledPixmap( 0, 0, _width, height(),
                               m_pTreeView->props()->bgPixmap(),
                               0, 0 ); // ?
  }

  // Now prevent QListViewItem::paintCell from drawing a white background
  QColorGroup cg( _cg );
  // I hope color0 is transparent :-))
  cg.setColor( QColorGroup::Base, Qt::color0 );

  QListViewItem::paintCell( _painter, _cg, _column, _width, _alignment );
}

/**************************************************************
 *
 * KonqTreeViewDir
 *
 **************************************************************/

KonqTreeViewDir::KonqTreeViewDir( KonqTreeViewWidget *_parent, KFileItem* _fileitem )
  : KonqTreeViewItem( _parent, _fileitem )
{
  m_pTreeView->addSubDir( _fileitem->url(), this );

  m_bComplete = false;
}

KonqTreeViewDir::KonqTreeViewDir( KonqTreeViewWidget *_treeview, KonqTreeViewDir * _parent, KFileItem* _fileitem )
  : KonqTreeViewItem( _treeview, _parent, _fileitem )
{
  m_pTreeView->addSubDir( _fileitem->url(), this );

  m_bComplete = false;
}

KonqTreeViewDir::~KonqTreeViewDir()
{
  if ( m_pTreeView )
    m_pTreeView->removeSubDir( m_fileitem->url() );
}

void KonqTreeViewDir::setup()
{
  setExpandable( true );
  QListViewItem::setup();
}

void KonqTreeViewDir::setOpen( bool _open )
{
  if ( _open && !m_bComplete ) // complete it before opening
    m_pTreeView->openSubFolder( m_fileitem->url(), this );

  QListViewItem::setOpen( _open );
}

QString KonqTreeViewDir::url( int _trailing )
{
  return m_fileitem->url().url( _trailing );
}

