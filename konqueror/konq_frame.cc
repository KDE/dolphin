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
#include <qwhatsthis.h>
#include <qtimer.h>

#include <kapp.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kpixmap.h>
#include <kprogress.h>
#include <klocale.h>
#include <kseparator.h>

#include <kparts/browserextension.h>
#include <kparts/event.h>
#include "konq_frame.h"
#include "konq_view.h"
#include "konq_viewmgr.h"
#include "konq_mainwindow.h"

#include <assert.h>

#define DEFAULT_HEADER_HEIGHT 13

void KonqCheckBox::paintEvent(QPaintEvent *)
{
    //static QPixmap indicator_anchor( UserIcon( "indicator_anchor" ) );
    static QPixmap indicator_connect( UserIcon( "indicator_connect" ) );
    static QPixmap indicator_noconnect( UserIcon( "indicator_noconnect" ) );

   QPainter p(this);

   if (isOn() || isDown())
      p.drawPixmap(0,0,indicator_connect);
   else
      p.drawPixmap(0,0,indicator_noconnect);
}

KonqFrameStatusBar::KonqFrameStatusBar( KonqFrame *_parent, const char *_name )
:QWidget( _parent, _name )
,m_pParentKonqFrame( _parent )
,m_yOffset(0)
,m_showLed(true)
{
   m_pStatusLabel = new QLabel( this );
   m_pStatusLabel->show();
   m_pStatusLabel->installEventFilter(this);

   m_pLinkedViewCheckBox = new KonqCheckBox( this, "m_pLinkedViewCheckBox" );
   m_pLinkedViewCheckBox->show();
   QWhatsThis::add( m_pLinkedViewCheckBox,
                    i18n("Checking this box on at least two views sets those views as 'linked'. "
                         "Then, when you change directories in one view, the other views "
                         "linked with it will automatically update to show the current directory. "
                         "This is especially useful with different types of views, such as a "
                         "directory tree with an icon view or detailed view, and possibly a "
                         "terminal emulator window." ) );

   int h=fontMetrics().height()+2;
   if (h<DEFAULT_HEADER_HEIGHT ) h=DEFAULT_HEADER_HEIGHT;
   setFixedHeight(h);
   m_yOffset=(h-13)/2;

   //m_pLinkedViewCheckBox->setGeometry(20,m_yOffset,13,13);
   m_pLinkedViewCheckBox->setFocusPolicy(NoFocus);
   m_pStatusLabel->setGeometry(40,0,50,h);

   connect(m_pLinkedViewCheckBox,SIGNAL(toggled(bool)),this,SIGNAL(linkedViewClicked(bool)));

   m_progressBar = new KProgress( 0, 100, 0, KProgress::Horizontal, this );
   m_progressBar->hide();
  //m_statusBar->insertWidget( m_progressBar, 120, STATUSBAR_LOAD_ID );
   m_msgTimer = 0;
}

KonqFrameStatusBar::~KonqFrameStatusBar()
{
}

void KonqFrameStatusBar::resizeEvent( QResizeEvent* )
{
   m_progressBar->setGeometry( width()-160, 0, 140, height() );
   m_pLinkedViewCheckBox->move( width()-15, m_yOffset ); // right justify
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
   KActionCollection *actionColl = m_pParentKonqFrame->childView()->mainWindow()->actionCollection();

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

void KonqFrameStatusBar::message( const QString &msg )
{
  if ( !m_msgTimer )
  {
    m_msgTimer = new QTimer( this, "msgtimer" );
    connect( m_msgTimer, SIGNAL( timeout() ),
	     this, SLOT( slotClear() ) );
  }
  else if ( m_msgTimer->isActive() )
    m_msgTimer->stop();

  QString saveMsg = m_savedMessage;

  slotDisplayStatusText( msg );

  m_savedMessage = saveMsg;

  m_msgTimer->start( 2000 );
}

void KonqFrameStatusBar::slotDisplayStatusText(const QString& text)
{
   //kdDebug(1202)<<"KongFrameHeader::slotDisplayStatusText("<<text<<")"<<endl;
   m_pStatusLabel->resize(fontMetrics().width(text),fontMetrics().height());
   m_pStatusLabel->setText(text);
   m_savedMessage = text;

   if ( m_msgTimer && m_msgTimer->isActive() )
     m_msgTimer->stop();
}

void KonqFrameStatusBar::slotClear()
{
  slotDisplayStatusText( m_savedMessage );
}

void KonqFrameStatusBar::slotLoadingProgress( int percent )
{
  if ( percent != -1 && percent != 100 ) // hide on 100 too
  {
    if ( !m_progressBar->isVisible() )
      m_progressBar->show();
  }
  else
    m_progressBar->hide();

  m_progressBar->setValue( percent );
  //m_statusBar->changeItem( 0L, STATUSBAR_SPEED_ID );
  //m_statusBar->changeItem( 0L, STATUSBAR_MSG_ID );
}

void KonqFrameStatusBar::slotSpeedProgress( int bytesPerSecond )
{
  QString sizeStr;

  if ( bytesPerSecond > 0 )
    sizeStr = KIO::convertSize( bytesPerSecond ) + QString::fromLatin1( "/s" );
  else
    sizeStr = i18n( "stalled" );

  //m_statusBar->changeItem( sizeStr, STATUSBAR_SPEED_ID );
  slotDisplayStatusText( sizeStr ); // let's share the same label...
}

void KonqFrameStatusBar::slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *newOne)
{
   if (newOne!=0)
      connect(newOne,SIGNAL(setStatusBarText(const QString &)),this,SLOT(slotDisplayStatusText(const QString&)));
   slotDisplayStatusText( QString::null );
}

void KonqFrameStatusBar::showActiveViewIndicator( bool b )
{
    m_showLed = b;
    repaint();
}

void KonqFrameStatusBar::showLinkedViewIndicator( bool b )
{
    if (b)
      m_pLinkedViewCheckBox->show();
    else
      m_pLinkedViewCheckBox->hide();
    //repaint();
}

void KonqFrameStatusBar::setLinkedView( bool b )
{
    m_pLinkedViewCheckBox->setChecked( b );
}

void KonqFrameStatusBar::paintEvent(QPaintEvent* e)
{
   static QPixmap indicator_viewactive( UserIcon( "indicator_viewactive" ) );
   static QPixmap indicator_empty( UserIcon( "indicator_empty" ) );

   if (!isVisible()) return;
   QWidget::paintEvent(e);
   if (!m_showLed) return;
   bool hasFocus = m_pParentKonqFrame->isActivePart();
   // Can't happen
   //   if ( m_pParentKonqFrame->childView()->passiveMode() )
   //   hasFocus = false;
   QPainter p(this);
   if (hasFocus)
      p.drawPixmap(4,m_yOffset,indicator_viewactive);
   else
      p.drawPixmap(4,m_yOffset,indicator_empty);
}

//###################################################################

KonqFrame::KonqFrame( KonqFrameContainer *_parentContainer, const char *_name )
:QWidget(_parentContainer,_name)
{
   m_pLayout = 0L;
   m_pView = 0L;

   // add the frame statusbar to the layout
   m_pStatusBar = new KonqFrameStatusBar( this, "KonquerorFrameStatusBar");
   connect(m_pStatusBar, SIGNAL(clicked()), this, SLOT(slotStatusBarClicked()));
   connect( m_pStatusBar, SIGNAL( linkedViewClicked( bool ) ), this, SLOT( slotLinkedViewClicked( bool ) ) );
   m_separator = 0;

#ifdef METAVIEWS
   m_metaViewLayout = 0;
   m_metaViewFrame = new QFrame( this, "metaviewframe" );
   m_metaViewFrame->show();
#endif
}

KonqFrame::~KonqFrame()
{
  kdDebug(1202) << "KonqFrame::~KonqFrame() " << this << endl;
  //delete m_pLayout;
}

bool KonqFrame::isActivePart()
{
  return ( m_pView &&
           static_cast<KonqView*>(m_pView) == m_pView->mainWindow()->currentView() );
}

void KonqFrame::listViews( ChildViewList *viewList )
{
  viewList->append( childView() );
}

void KonqFrame::saveConfig( KConfig* config, const QString &prefix, bool saveURLs, int /*id*/, int /*depth*/ )
{
  config->writeEntry( QString::fromLatin1( "URL" ).prepend( prefix ),
                      saveURLs ? childView()->url().url() : QString::null );
  config->writeEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), childView()->serviceType() );
  config->writeEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), childView()->service()->name() );
  config->writeEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), childView()->passiveMode() );
  config->writeEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), childView()->linkedView() );
}

KParts::ReadOnlyPart *KonqFrame::attach( const KonqViewFactory &viewFactory )
{
   KonqViewFactory factory( viewFactory );

   // Note that we set the parent to 0.
   // We don't want that deleting the widget deletes the part automatically
   // because we already have that taken care of in KParts...

#ifdef METAVIEWS
   m_pPart = factory.create( m_metaViewFrame, "view widget", 0, "child view" );
#else
   m_pPart = factory.create( this, "view widget", 0, "child view" );
#endif

   assert( m_pPart->widget() );

   attachInternal();

   m_pStatusBar->slotConnectToNewView(0, 0,m_pPart);
   return m_pPart;
}

void KonqFrame::attachInternal()
{
   kdDebug(1202) << "KonqFrame::attachInternal()" << endl;
   if (m_pLayout) delete m_pLayout;

   m_pLayout = new QVBoxLayout( this, 0, -1, "KonqFrame's QVBoxLayout" );

#ifdef METAVIEWS
   if ( m_metaViewLayout ) delete m_metaViewLayout;
   m_metaViewFrame->setFrameStyle( QFrame::NoFrame );
   m_metaViewFrame->setLineWidth( 0 );
   //   m_metaViewFrame->setFrameStyle( QFrame::Panel | QFrame::Raised );
   //      m_metaViewFrame->setLineWidth( 50 );

   m_metaViewLayout = new QVBoxLayout( m_metaViewFrame );
   m_metaViewLayout->setMargin( m_metaViewFrame->frameWidth() );
   m_metaViewLayout->addWidget( m_pPart->widget() );

   m_pLayout->addWidget( m_metaViewFrame );
#else
   m_pLayout->addWidget( m_pPart->widget() );
#endif

   m_pLayout->addWidget( m_pStatusBar );
   m_pPart->widget()->show();
   if ( m_pView->mainWindow()->fullScreenMode() )
     m_pView->mainWindow()->attachToolbars( this );
   else
     m_pStatusBar->show();
   m_pLayout->activate();
}

void KonqFrame::setView( KonqView* child )
{
   m_pView = child;
   if (m_pView)
   {
     connect(m_pView,SIGNAL(sigPartChanged(KonqView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)),
             m_pStatusBar,SLOT(slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)));
     //connect(m_pView->view(),SIGNAL(setStatusBarText(const QString &)),
     //m_pHeader,SLOT(slotDisplayStatusText(const QString&)));
   }
};

KonqFrameContainer* KonqFrame::parentContainer()
{
   if( parentWidget()->isA("KonqFrameContainer") )
      return static_cast<KonqFrameContainer*>(parentWidget());
   else
      return 0L;
}

void KonqFrame::reparentFrame( QWidget* parent, const QPoint & p, bool showIt )
{
   QWidget::reparent( parent, p, showIt );
}

void KonqFrame::slotStatusBarClicked()
{
  if ( !isActivePart() )
    part()->widget()->setFocus(); // Will change the active part
}

void KonqFrame::slotLinkedViewClicked( bool mode )
{
  if (m_pView->mainWindow()->viewCount() == 2)
  {
    // Two views : link both
    KonqMainWindow::MapViews mapViews = m_pView->mainWindow()->viewMap();
    KonqMainWindow::MapViews::Iterator it = mapViews.begin();
    (*it)->setLinkedView( mode );
    ++it;
    (*it)->setLinkedView( mode );
  }
  else // Normal case : just this view
    m_pView->setLinkedView( mode );
}

void
KonqFrame::paintEvent( QPaintEvent* )
{
   m_pStatusBar->repaint();
}

void KonqFrame::detachMetaView()
{
  if ( m_separator )
    delete m_separator;
  m_separator = 0;
}

#ifdef METAVIEWS
void KonqFrame::attachMetaView( KParts::ReadOnlyPart *view, bool enableMetaViewFrame, const QMap<QString,QVariant> &framePropertyMap )
{
//  m_separator = new KSeparator( this );
//  m_pLayout->insertWidget( 0, m_separator );
//  m_pLayout->insertWidget( 0, view->widget() );
  m_metaViewLayout->insertWidget( 0, view->widget() );
  if ( enableMetaViewFrame )
  {
    QMapConstIterator<QString,QVariant> it = framePropertyMap.begin();
    QMapConstIterator<QString,QVariant> end = framePropertyMap.end();
    for (; it != end; ++it )
      m_metaViewFrame->setProperty( it.key(), it.data() );

    m_metaViewLayout->setMargin( m_metaViewFrame->frameWidth() );
  }
}
#else
void KonqFrame::attachMetaView( KParts::ReadOnlyPart *, bool, const QMap<QString,QVariant> & )
{
  kdError(1202) << "Meta views not supported" << endl;
}
#endif

//###################################################################

KonqFrameContainer::KonqFrameContainer( Orientation o,
					QWidget* parent,
					const char * name)
  : QSplitter( o, parent, name)
{
  m_pFirstChild = 0L;
  m_pSecondChild = 0L;
}

KonqFrameContainer::~KonqFrameContainer()
{
    kdDebug(1202) << "KonqFrameContainer::~KonqFrameContainer() " << this << " - " << className() << endl;
}

void KonqFrameContainer::listViews( ChildViewList *viewList )
{
   if( m_pFirstChild )
      m_pFirstChild->listViews( viewList );

   if( m_pSecondChild )
      m_pSecondChild->listViews( viewList );
}

void KonqFrameContainer::saveConfig( KConfig* config, const QString &prefix, bool saveURLs, int id, int depth )
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
    firstChild()->saveConfig( config, newPrefix, saveURLs, id, depth + 1 );
  }

  if( secondChild() ) {
    QString newPrefix = secondChild()->frameType() + QString("%1").arg( idSecond );
    newPrefix.append( '_' );
    secondChild()->saveConfig( config, newPrefix, saveURLs, idSecond, depth + 1 );
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
    return static_cast<KonqFrameContainer*>(parentWidget());
  else
    return 0L;
}

void KonqFrameContainer::reparentFrame( QWidget* parent, const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, p, showIt );
}

void KonqFrameContainer::swapChildren()
{
  KonqFrameBase *firstCh = m_pFirstChild;
  m_pFirstChild = m_pSecondChild;
  m_pSecondChild = firstCh;
}

// Ok, here it goes. When we receive the event for a KonqFrame[Container] that has been
// deleted - it's from the QObject destructor, so isA won't work. The class name is "QObject" !
// This is why childEvent only catches child insertion, and there is a manual removeChildFrame.
void KonqFrameContainer::childEvent( QChildEvent * ce )
{
  //kdDebug(1202) << "KonqFrameContainer::childEvent this" << this << endl;

  if( ce->type() == QEvent::ChildInserted )
  {
    KonqFrameBase* castChild = 0L;
    if( ce->child()->isA("KonqFrame") )
      castChild = static_cast< KonqFrame* >(ce->child());
    else if( ce->child()->isA("KonqFrameContainer") )
      castChild = static_cast< KonqFrameContainer* >(ce->child());

    if (castChild)
    {
        kdDebug(1202) << "KonqFrameContainer " << this << ": child " << castChild << " inserted" << endl;
        if( !m_pFirstChild )
            m_pFirstChild = castChild;

        else if( !m_pSecondChild )
            m_pSecondChild = castChild;

        else
            kdWarning(1202) << this << " already has two children..."
                            << m_pFirstChild << " and " << m_pSecondChild << endl;
    }
  }
  QSplitter::childEvent( ce );
}

void KonqFrameContainer::removeChildFrame( KonqFrameBase * frame )
{
  kdDebug(1202) << "KonqFrameContainer::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;
  if( m_pFirstChild == frame )
    m_pFirstChild = 0L;

  else if( m_pSecondChild == frame )
    m_pSecondChild = 0L;

  else
    kdWarning(1202) << this << " Can't find this child:" << frame << endl;
}

#include "konq_frame.moc"
