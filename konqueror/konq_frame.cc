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
#include <klocale.h>

#include <kparts/browserextension.h>
#include <kparts/event.h>
#include "konq_frame.h"
#include "konq_childview.h"
#include "konq_viewmgr.h"
#include "konq_mainview.h"

#include <assert.h>

//these both have white background
//#include "anchor.xpm"
//#include "white.xpm"
#include "green.xpm"
//these both have very bright grey background
#include "lightgrey.xpm"
#include "anchor_grey.xpm"

#define DEFAULT_HEADER_HEIGHT 13

//#include <iostream.h>

void KonqCheckBox::paintEvent(QPaintEvent *)
{
    static QPixmap anchor_grey(anchor_grey_xpm);
    static QPixmap lightgrey(lightgrey_xpm);

   QPainter p(this);

   if (isOn() || isDown())
      p.drawPixmap(0,0,anchor_grey);
   else
      p.drawPixmap(0,0,lightgrey);
}

KonqFrameStatusBar::KonqFrameStatusBar( KonqFrame *_parent, const char *_name )
:QWidget( _parent, _name )
,m_pParentKonqFrame( _parent )
,statusLabel(this)
,m_yOffset(0)
,m_showLed(true)
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
   m_pPassiveModeCheckBox->setFocusPolicy(NoFocus);
   statusLabel.setGeometry(40,0,50,h);

   connect(m_pPassiveModeCheckBox,SIGNAL(toggled(bool)),this,SIGNAL(passiveModeChange(bool)));
}

void KonqFrameStatusBar::mousePressEvent( QMouseEvent* event )
{
   QWidget::mousePressEvent( event );
   if ( !m_pParentKonqFrame->childView()->passiveMode() )
   {
      emit clicked();
      update();
   }
   if (event->button()==RightButton)
      splitFrameMenu();
}

void KonqFrameStatusBar::splitFrameMenu()
{
   QActionCollection *actionColl = m_pParentKonqFrame->childView()->mainView()->actionCollection();

   QPopupMenu menu;

   actionColl->action( "splitviewh" )->plug( &menu );
   actionColl->action( "splitviewv" )->plug( &menu );
   actionColl->action( "splitwindowh" )->plug( &menu );
   actionColl->action( "splitwindowv" )->plug( &menu );
   actionColl->action( "removeview" )->plug( &menu );

   menu.exec(QCursor::pos());
}

bool KonqFrameStatusBar::eventFilter(QObject*,QEvent *e)
{
   if (e->type()==QEvent::MouseButtonPress)
   {
      emit clicked();
      update();
      if ( ((QMouseEvent*)e)->button()==RightButton)
         splitFrameMenu();
      return TRUE;
   }
   return FALSE;
}

void KonqFrameStatusBar::slotDisplayStatusText(const QString& text)
{
   //kdDebug()<<"KongFrameHeader::slotDisplayStatusText("<<text<<")"<<endl;
   statusLabel.resize(fontMetrics().width(text),13);
   statusLabel.setText(text);
}

void KonqFrameStatusBar::slotConnectToNewView(KParts::ReadOnlyPart *,KParts::ReadOnlyPart *newOne)
{
   if (newOne!=0)
      connect(newOne,SIGNAL(setStatusBarText(const QString &)),this,SLOT(slotDisplayStatusText(const QString&)));
   slotDisplayStatusText( QString::null );
}

void KonqFrameStatusBar::showStuff()
{
    m_pPassiveModeCheckBox->show();
    m_showLed = true;
    repaint();
}

void KonqFrameStatusBar::hideStuff()
{
    m_pPassiveModeCheckBox->hide();
    m_showLed = false;
    repaint();
}

void KonqFrameStatusBar::paintEvent(QPaintEvent* e)
{
    static QPixmap green(green_xpm);
    static QPixmap lightgrey(lightgrey_xpm);

   if (!isVisible()) return;
   QWidget::paintEvent(e);
   if (!m_showLed) return;
   bool hasFocus = m_pParentKonqFrame->isActivePart();
   if ( m_pParentKonqFrame->childView()->passiveMode() )
      hasFocus = false;
   QPainter p(this);
   if (hasFocus)
      p.drawPixmap(4,m_yOffset,green);
   else
   {
      p.drawPixmap(4,m_yOffset,lightgrey);
   };
}

//###################################################################

KonqFrame::KonqFrame( KonqFrameContainer *_parentContainer, const char *_name )
:QWidget(_parentContainer,_name)
{
   m_pLayout = 0L;
   m_pChildView = 0L;

   // add the frame statusbar to the layout
   m_pStatusBar = new KonqFrameStatusBar( this, "KonquerorFrameStatusBar");
   QObject::connect(m_pStatusBar, SIGNAL(clicked()), this, SLOT(slotStatusBarClicked()));
   connect( m_pStatusBar, SIGNAL( passiveModeChange( bool ) ), this, SLOT( slotPassiveModeChange( bool ) ) );
}

KonqFrame::~KonqFrame()
{
  //delete m_pLayout;
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
   KonqViewFactory factory( viewFactory );

   m_pView = factory.create( this, "child view" );

   assert( m_pView->widget() );

   attachInternal();

   m_pStatusBar->slotConnectToNewView(0,m_pView);
   return m_pView;
}

void KonqFrame::attachInternal()
{
   kdDebug(1202) << "KonqFrame::attachInternal()" << endl;
   if (m_pLayout) delete m_pLayout;

   m_pLayout = new QVBoxLayout( this, 0, -1, "KonqFrame's QVBoxLayout" );

   m_pLayout->addWidget( m_pView->widget() );
   m_pLayout->addWidget( m_pStatusBar );
   m_pView->widget()->show();
   if ( m_pChildView->mainView()->fullScreenMode() )
     m_pChildView->mainView()->attachToolbars( this );
   else
     m_pStatusBar->show();
   m_pLayout->activate();
}

void KonqFrame::setChildView( KonqChildView* child )
{
   m_pChildView = child;
   if (m_pChildView!=0)
   {
      connect(m_pChildView,SIGNAL(sigViewChanged(KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)),m_pStatusBar,SLOT(slotConnectToNewView(KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)));
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

void KonqFrame::slotStatusBarClicked()
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
   m_pStatusBar->repaint();
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
