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

#include "kfmbrowser.h"
#include "kfmgui.h"
#include "kfmguiprops.h"
#include "krun.h"
#include "kfmview.h"
#include "kmimetypes.h"

#include <sys/stat.h>

#include <qstring.h>
#include <string.h>
#include <qstrlist.h>

#include <kcursor.h>
#include <khtml.h>
#include <kapp.h>

#include <k2url.h>
#include <kio_error.h>

KfmBrowser::KfmBrowser( QWidget *_parent, KfmView *_view, const char *_name, KBrowser *_parent_browser )
  : KBrowser( _parent, _name, _parent_browser )
{
  m_pView = _view;
  initConfig();

  connect( this, SIGNAL( mousePressed( const char*, const QPoint&, int ) ),
	   this, SLOT( slotMousePressed( const char*, const QPoint&, int ) ) );

}

KfmBrowser::~KfmBrowser()
{
}

void KfmBrowser::initConfig()
{
  KfmGuiProps *props = m_pView->gui()->props();
  KHTMLWidget* htmlWidget = getKHTMLWidget();

  htmlWidget->setDefaultBGColor( props->bgColor() );
  htmlWidget->setDefaultTextColors( props->textColor(), 
				    props->linkColor(),
				    props->vLinkColor() );
  htmlWidget->setStandardFont( props->stdFontName() );
  htmlWidget->setFixedFont( props->fixedFontName() );

  htmlWidget->setUnderlineLinks( props->underlineLink() );

  if ( props->changeCursor() )
    htmlWidget->setURLCursor( KCursor().handCursor() );
  else
    htmlWidget->setURLCursor( KCursor().arrowCursor() );
}

void KfmBrowser::slotNewWindow( const char *_url )
{
  (void)new KRun( _url, 0, false );
}

KBrowser* KfmBrowser::createFrame( QWidget *_parent, const char *_name )
{
  return ( new KfmBrowser( _parent, m_pView, _name, this ) );
}

void KfmBrowser::slotMousePressed( const char* _url, const QPoint &_global, int _button )
{
  QString url = _url;

  if ( !_url )
    if ( !m_strURL )
      return;
    else
      url = m_strURL;

  if ( _button == RightButton )
  {
    K2URL u( url );
    QStrList lst;
    lst.append( url );
    
    mode_t mode = 0;
    if ( u.isLocalFile() )
      {    
	struct stat buff;
	if ( stat( u.path(), &buff ) == -1 )
	  {
	    kioErrorDialog( ERR_COULD_NOT_STAT, url );
	    return;
	  }
	mode = buff.st_mode;
      }
    
    m_pView->popupMenu( _global, lst, mode, u.isLocalFile() );
  }
}

void KfmBrowser::slotOnURL( const char *_url )
{
  if ( !_url )
  {    
    m_pView->gui()->setStatusBarText( "" );
    return;
  }

  K2URL url( _url );
  QString com;
  
  KMimeType *typ = KMimeType::findByURL( url );
  
  if ( typ )
    com = typ->comment( url, false );
  
  if ( url.isMalformed() )
  {
    m_pView->gui()->setStatusBarText( _url );
    return;
  }

  QString decodedPath( url.path() );
  QString decodedName( url.filename( true ).c_str() );
	
  struct stat buff;
  stat( decodedPath, &buff );
  
  struct stat lbuff;
  lstat( decodedPath, &lbuff );
  QString text;
  QString text2;
  text = decodedName.copy(); // copy to change it
  text2 = text;
  text2.detach();
	
  if ( url.isLocalFile() )
  {
    if (S_ISLNK( lbuff.st_mode ) )
    {
      QString tmp;
      if ( com.isNull() )
	tmp = i18n( "Symbolic Link");
      else
	tmp.sprintf(i18n("%s (Link)"), com.data() );
      char buff_two[1024];
      text += "->";
      int n = readlink ( decodedPath, buff_two, 1022);
      if (n == -1)
      {
	m_pView->gui()->setStatusBarText( text2 + "  " + tmp );
	return;
      }
      buff_two[n] = 0;
      text += buff_two;
      text += "  ";
      text += tmp.data();
    }
    else if ( S_ISREG( buff.st_mode ) )
    {
      text += " ";
      if (buff.st_size < 1024)
	text.sprintf( "%s (%ld %s)", 
		      text2.data(), (long) buff.st_size,
		      i18n("bytes"));
      else
      {
	float d = (float) buff.st_size/1024.0;
	text.sprintf( "%s (%.2f K)", text2.data(), d);
      }
      text += "  ";
      text += com.data();
    }
    else if ( S_ISDIR( buff.st_mode ) )
    {
      text += "/  ";
      text += com.data();
    }
    else
    {
      text += "  ";
      text += com.data();
    }	  
    m_pView->gui()->setStatusBarText( text );
  }
  else
    m_pView->gui()->setStatusBarText( url.url().c_str() );
}

bool KfmBrowser::mousePressedHook( const char *_url, const char *_target, QMouseEvent *_mouse, bool _isselected )
{
  emit gotFocus();

  return KBrowser::mousePressedHook( _url, _target, _mouse, _isselected );
}

void KfmBrowser::setFocus()
{
  emit gotFocus();

  KBrowser::setFocus();
}

#include "kfmicons.h"

KHTMLEmbededWidget* KfmBrowser::newEmbededWidget( QWidget* _parent, const char *_name, const char *_src, const char *_type,
						  int _marginwidth, int _marginheight, int _frameborder, bool _noresize )
{
  KfmEmbededFrame *e = new KfmEmbededFrame( _parent, _frameborder, _noresize );
  KfmIconView* icons = new KfmIconView( e, m_pView );
  e->setChild( icons );
  if ( _src == 0L || *_src == 0 )
  {
    QString url = m_pView->workingURL();
    if ( url.isEmpty() )
      url = m_pView->currentURL();
    if ( !url.isEmpty() )
      icons->openURL( url );
  }
  icons->openURL( _src );

  return e;
}

/**********************************************
 *
 * KfmEmbededFrame
 *
 **********************************************/

KfmEmbededFrame::KfmEmbededFrame( QWidget *_parent, int _frameborder, bool _allowresize )
  : KHTMLEmbededWidget( _parent, _frameborder, _allowresize )
{
  m_pChild = 0L;
}
  
void KfmEmbededFrame::setChild( QWidget *_widget )
{
  if ( m_pChild )
    delete m_pChild;
  
  m_pChild = _widget;
  resizeEvent( 0L );
}

QWidget* KfmEmbededFrame::child()
{
  return m_pChild;
}

void KfmEmbededFrame::resizeEvent( QResizeEvent *_ev )
{
  if ( m_pChild == 0L )
    return;
  
  m_pChild->setGeometry( 0, 0, width(), height() );
}


#include "kfmbrowser.moc"
