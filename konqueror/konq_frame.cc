/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

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
#include <math.h>

#include <qpainter.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qlayout.h>
#include <qsplitter.h>

#include <kapp.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kpixmap.h>

#include <kparts/browserextension.h>
#include "konq_frame.h"
#include "konq_childview.h"
#include "konq_viewmgr.h"

#include <assert.h>

//these both have white background
//#include "anchor.xpm"
//#include "white.xpm"
#include "green.xpm"
//these both have very bright grey background
#include "lightgrey.xpm"
#include "anchor_grey.xpm"

#define DEFAULT_HEADER_HEIGHT 13

#include <iostream.h>

void KonqCheckBox::paintEvent(QPaintEvent *)
{
   QPainter p(this);

   if (isOn() || isDown())
      p.drawPixmap(0,0,QPixmap(anchor_grey_xpm));
   else
      p.drawPixmap(0,0,QPixmap(lightgrey_xpm));
}

KonqFrameHeader::KonqFrameHeader( KonqFrame *_parent, const char *_name )
:QWidget( _parent, _name )
,m_pParentKonqFrame( _parent )
,statusLabel(this)
,m_yOffset(0)
{
   m_pPassiveModeCheckBox = new KonqCheckBox( this );
   m_pPassiveModeCheckBox->show();
   statusLabel.show();
   statusLabel.installEventFilter(this);

   int h=fontMetrics().height()+2;
   if (h<DEFAULT_HEADER_HEIGHT ) h=DEFAULT_HEADER_HEIGHT;
   setFixedHeight(h);
   m_yOffset=(h-13)/2;

   m_pPassiveModeCheckBox->setGeometry(20,m_yOffset,13,13);
   statusLabel.setGeometry(40,0,50,h);

   connect(m_pPassiveModeCheckBox,SIGNAL(toggled(bool)),this,SIGNAL(passiveModeChange(bool)));
}

void KonqFrameHeader::mousePressEvent( QMouseEvent* event )
{
   QWidget::mousePressEvent( event );
   if ( !m_pParentKonqFrame->childView()->passiveMode() )
   {
      emit headerClicked();
      update();
   }
}

bool KonqFrameHeader::eventFilter(QObject*,QEvent *e)
{
   if (e->type()==QEvent::MouseButtonPress)
   {
      emit headerClicked();
      update();
      return TRUE;
   };
   return FALSE;
};

void KonqFrameHeader::slotDisplayStatusText(const QString& text)
{
   cerr<<"KongFrameHeader::slotDisplayStatusText("<<text<<")"<<endl;
   statusLabel.resize(fontMetrics().width(text),13);
   statusLabel.setText(text);
};

void KonqFrameHeader::slotConnectToNewView(KParts::ReadOnlyPart *oldOne,KParts::ReadOnlyPart *newOne)
{
   if (newOne!=0)
      connect(newOne,SIGNAL(setStatusBarText(const QString &)),this,SLOT(slotDisplayStatusText(const QString&)));
};

void KonqFrameHeader::paintEvent(QPaintEvent* e)
{
   if (!isVisible()) return;
   QWidget::paintEvent(e);
   bool hasFocus = m_pParentKonqFrame->isActivePart();
   if ( m_pParentKonqFrame->childView()->passiveMode() )
      hasFocus = false;
   QPainter p(this);
   if (hasFocus)
      p.drawPixmap(4,m_yOffset,QPixmap(green_xpm));
   else
   {
      p.drawPixmap(4,m_yOffset,QPixmap(lightgrey_xpm));
   };
};

//###################################################################

KonqFrame::KonqFrame( KonqFrameContainer *_parentContainer, const char *_name )
:QWidget(_parentContainer,_name)
{
   m_pLayout = 0L;
   m_pChildView = 0L;

   // add the frame header to the layout
   m_pHeader = new KonqFrameHeader( this, "KonquerorFrameHeader");
   QObject::connect(m_pHeader, SIGNAL(headerClicked()), this, SLOT(slotHeaderClicked()));
   connect( m_pHeader, SIGNAL( passiveModeChange( bool ) ), this, SLOT( slotPassiveModeChange( bool ) ) );
}

KParts::ReadOnlyPart * KonqFrame::view( void )
{
  return m_pView;
}

bool KonqFrame::isActivePart()
{
  return ( (KParts::ReadOnlyPart *)m_pView == m_pChildView->mainView()->currentView() );
}

void KonqFrame::listViews( ChildViewList *viewList )
{
  viewList->append( childView() );
}

void KonqFrame::saveConfig( KConfig* config, const QString &prefix, int /*id*/, int /*depth*/ )
{
  config->writeEntry( QString::fromLatin1( "URL" ).prepend( prefix ), childView()->url().url() );
  config->writeEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), childView()->serviceType() );
  config->writeEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), childView()->service()->name() );
  config->writeEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), childView()->passiveMode() );
}

KParts::ReadOnlyPart *KonqFrame::attach( const KonqViewFactory &viewFactory )
{
   if (m_pLayout) delete m_pLayout;

   KonqViewFactory factory( viewFactory );

   m_pLayout = new QVBoxLayout( this );

   m_pView = factory.create( this, 0L );

   assert( m_pView->widget() );

   m_pLayout->addWidget( m_pView->widget() );
   m_pLayout->addWidget( m_pHeader );
   m_pView->widget()->show();
   m_pHeader->show();
   m_pLayout->activate();

   connect(m_pView,SIGNAL(setStatusBarText(const QString &)),m_pHeader,SLOT(slotDisplayStatusText(const QString&)));

   return m_pView;
}

void KonqFrame::setChildView( KonqChildView* child )
{
   m_pChildView = child;
   if (m_pChildView!=0)
   {
      connect(m_pChildView,SIGNAL(sigViewChanged(KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)),m_pHeader,SLOT(slotConnectToNewView(KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)));
      //connect(m_pChildView->view(),SIGNAL(setStatusBarText(const QString &)),m_pHeader,SLOT(slotDisplayStatusText(const QString&)));
   };
};

KonqFrameContainer* KonqFrame::parentContainer()
{
   if( parentWidget()->isA("KonqFrameContainer") )
      return (KonqFrameContainer*)parentWidget();
   else
      return 0L;
}

void KonqFrame::reparent( QWidget* parent, WFlags f,
		     const QPoint & p, bool showIt )
{
   QWidget::reparent( parent, f, p, showIt );
}

void KonqFrame::slotHeaderClicked()
{
  if ( m_pView != m_pChildView->mainView()->viewManager()->activePart() )
     m_pChildView->mainView()->viewManager()->setActivePart( m_pView );
}

void KonqFrame::slotPassiveModeChange( bool mode )
{
   m_pChildView->setPassiveMode( mode );
}

void
KonqFrame::paintEvent( QPaintEvent* )
{
   m_pHeader->repaint();
}

//###################################################################

KonqFrameContainer::KonqFrameContainer( Orientation o,
					QWidget* parent,
					const char * name)
  : QSplitter( o, parent, name)
{
  m_pFirstChild = 0L;
  m_pSecondChild = 0L;
}

void KonqFrameContainer::listViews( ChildViewList *viewList )
{
   if( firstChild() )
      firstChild()->listViews( viewList );

   if( secondChild() )
      secondChild()->listViews( viewList );
}

void KonqFrameContainer::saveConfig( KConfig* config, const QString &prefix, int id, int depth )
{
  int idSecond = id + (int)pow( 2, depth );

  //write children sizes
  config->writeEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ), sizes() );

  //write children
  QStringList strlst;
  if( firstChild() )
    strlst.append( firstChild()->frameType() + QString("%1").arg(idSecond - 1) );
  if( secondChild() )
    strlst.append( secondChild()->frameType() + QString("%1").arg( idSecond ) );

  config->writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  //write orientation
  QString o;
  if( orientation() == Qt::Horizontal )
    o = QString::fromLatin1("Horizontal");
  else if( orientation() == Qt::Vertical )
    o = QString::fromLatin1("Vertical");
  config->writeEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ), o );


  //write child configs
  if( firstChild() ) {
    QString newPrefix = firstChild()->frameType() + QString("%1").arg(idSecond - 1);
    newPrefix.append( '_' );
    firstChild()->saveConfig( config, newPrefix, id, depth + 1 );
  }

  if( secondChild() ) {
    QString newPrefix = secondChild()->frameType() + QString("%1").arg( idSecond );
    newPrefix.append( '_' );
    secondChild()->saveConfig( config, newPrefix, idSecond, depth + 1 );
  }
}

KonqFrameBase* KonqFrameContainer::otherChild( KonqFrameBase* child )
{
   if( firstChild() == child )
      return secondChild();
   else if( secondChild() == child )
      return firstChild();
   return 0L;
}

KonqFrameContainer* KonqFrameContainer::parentContainer()
{
  if( parentWidget()->isA("KonqFrameContainer") )
    return (KonqFrameContainer*)parentWidget();
  else
    return 0L;
}

void KonqFrameContainer::reparent( QWidget* parent, WFlags f, const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, f, p, showIt );
}

void KonqFrameContainer::childEvent( QChildEvent * ce )
{
   KonqFrameBase* castChild = 0L;

   if( ce->child()->isA("KonqFrame") )
      castChild = ( KonqFrame* )ce->child();
   else if( ce->child()->isA("KonqFrameContainer") )
      castChild = ( KonqFrameContainer* )ce->child();

   if( ce->type() == QEvent::ChildInserted )
   {

      if( castChild )
         if( !firstChild() )
         {
            setFirstChild( castChild );
         }
         else if( !secondChild() )
         {
            setSecondChild( castChild );
         }

   }
   else if( ce->type() == QEvent::ChildRemoved )
   {

      if( castChild )
      {
         if( firstChild() == ( KonqFrameBase* )castChild )
         {
            setFirstChild( 0L );
         }
         else if( secondChild() == ( KonqFrameBase* )castChild )
         {
            setSecondChild( 0L );
         }
      }
   }

   QSplitter::childEvent( ce );
}

#include "konq_frame.moc"
