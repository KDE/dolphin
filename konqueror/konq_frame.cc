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
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qtoolbutton.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kprogress.h>
#include <klocale.h>

#include "konq_frame.h"
#include "konq_view.h"
#include "konq_viewmgr.h"

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
//   m_msgTimer = 0;
}

KonqFrameStatusBar::~KonqFrameStatusBar()
{
}

void KonqFrameStatusBar::resizeEvent( QResizeEvent* )
{
   m_progressBar->setGeometry( width()-160, 0, 140, height() );
   m_pLinkedViewCheckBox->move( width()-15, m_yOffset ); // right justify
}

// I don't think this code _ever_ gets called!
// I don't want to remove it, though.  :-)
void KonqFrameStatusBar::mousePressEvent( QMouseEvent* event )
{
   QWidget::mousePressEvent( event );
   if ( !m_pParentKonqFrame->childView()->isPassiveMode() )
   {
      emit clicked();
      update();
   }
   if (event->button()==RightButton)
      splitFrameMenu();
}

void KonqFrameStatusBar::splitFrameMenu()
{
   KonqMainWindow * mw = m_pParentKonqFrame->childView()->mainWindow();

   // We have to ship the remove view action ourselves,
   // since this may not be the active view (passive view)
   KAction actRemoveView(i18n("Remove View"), 0, m_pParentKonqFrame, SLOT(slotRemoveView()), 0, "removethisview");
   //KonqView * nextView = mw->viewManager()->chooseNextView( m_pParentKonqFrame->childView() );
   actRemoveView.setEnabled( mw->mainViewsCount() > 1 || m_pParentKonqFrame->childView()->isToggleView() || m_pParentKonqFrame->childView()->isPassiveMode() );

   // For the rest, we borrow them from the main window
   // ###### might be not right for passive views !
   KActionCollection *actionColl = mw->actionCollection();

   QPopupMenu menu;

   actionColl->action( "splitviewh" )->plug( &menu );
   actionColl->action( "splitviewv" )->plug( &menu );
   menu.insertSeparator();
   if (m_pParentKonqFrame->childView()->isLockedLocation())
       actionColl->action( "unlock" )->plug( &menu );
   else
       actionColl->action( "lock" )->plug( &menu );

   actRemoveView.plug( &menu );

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
    /*
  if ( !m_msgTimer )
  {
    m_msgTimer = new QTimer( this, "msgtimer" );
    connect( m_msgTimer, SIGNAL( timeout() ),
             this, SLOT( slotClear() ) );
  }
  else if ( m_msgTimer->isActive() )
    m_msgTimer->stop();
    */

  QString saveMsg = m_savedMessage;

  slotDisplayStatusText( msg );

  m_savedMessage = saveMsg;

//  m_msgTimer->start( 2000 );
}

void KonqFrameStatusBar::slotDisplayStatusText(const QString& text)
{
   //kdDebug(1202)<<"KongFrameHeader::slotDisplayStatusText("<<text<<")"<<endl;
   m_pStatusLabel->resize(fontMetrics().width(text),fontMetrics().height());
   m_pStatusLabel->setText(text);
   m_savedMessage = text;

//   if ( m_msgTimer && m_msgTimer->isActive() )
//     m_msgTimer->stop();
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
    sizeStr = i18n( "Stalled" );

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
    m_pLinkedViewCheckBox->blockSignals( true );
    m_pLinkedViewCheckBox->setChecked( b );
    m_pLinkedViewCheckBox->blockSignals( false );
}

void KonqFrameStatusBar::paintEvent(QPaintEvent* e)
{
#ifndef NOINDICATOR
   static QPixmap indicator_viewactive( UserIcon( "indicator_viewactive" ) );
   static QPixmap indicator_empty( UserIcon( "indicator_empty" ) );
#endif

   if (!isVisible()) return;
   bool hasFocus = m_pParentKonqFrame->isActivePart();
   QPalette pal = palette();

   QColor bg = kapp->palette().active().background();
   pal.setColor( QColorGroup::Background,
                 m_showLed ? ( hasFocus ? kapp->palette().active().midlight()
                               : kapp->palette().active().mid() ) // active/inactive
                 : bg ); // only one view

   setPalette( pal );
   QWidget::paintEvent(e);
#ifndef NOINDICATOR
   if (m_showLed) {
     QPainter p(this);
     p.drawPixmap(4,m_yOffset,hasFocus ? indicator_viewactive : indicator_empty);
   }
#endif
}


KonqFrameHeader::KonqFrameHeader( KonqFrame *_parent, const char *_name )
:QWidget( _parent, _name )
,m_pParentKonqFrame( _parent )
{
   // this is the best font i could think of to use.
   QFont f = KGlobalSettings::generalFont();

   // do I have to worry about negative font sizes? (bc of the -1)
//   f.setPixelSize(f.pixelSize() - 1);



   m_pLayout = new QHBoxLayout( this, 0, -1, "KonqFrame's QVBoxLayout" );
   m_pHeaderLabel = new QLabel( this, "KonqFrameHeader label" );
   m_pHeaderLabel->setAlignment(AlignCenter);
   m_pHeaderLabel->setFrameStyle( QFrame::StyledPanel );
   m_pHeaderLabel->installEventFilter(this);
   m_pCloseButton = new QToolButton( this );
   m_pCloseButton->setAutoRaise(true);

   // button is square -- figure out the len of an edge
   int edge = m_pCloseButton->fontMetrics().height() + 5;

   m_pCloseButton->setMaximumHeight(edge);
   m_pCloseButton->setMaximumWidth(edge);
   m_pCloseButton->setMinimumWidth(edge);  //make it square as the width is smaller than the height

   f.setBold(false);
   m_pHeaderLabel->setFont(f);
   f.setBold(true);
   m_pCloseButton->setFont(f);


   m_pLayout->addWidget( m_pHeaderLabel );
   m_pLayout->addWidget( m_pCloseButton );

   m_pLayout->setStretchFactor( m_pHeaderLabel, 1 );
   m_pLayout->setStretchFactor( m_pCloseButton, 0 );

   m_pCloseButton->setText("x");


   m_pCloseButton->setFocusPolicy(NoFocus);
}

KonqFrameHeader::~KonqFrameHeader()
{
}

void KonqFrameHeader::setText(const QString &text)
{
    if( !isVisible() ) show();
    m_pHeaderLabel->setText(text);
}

void KonqFrameHeader::setAction( KAction *inAction )
{
    connect(m_pCloseButton, SIGNAL(clicked()), inAction, SLOT(activate()));
}


bool KonqFrameHeader::eventFilter(QObject*,QEvent *e)
{
   if (e->type()==QEvent::MouseButtonPress)
   {
      if ( ((QMouseEvent*)e)->button()==RightButton)
         showCloseMenu();
      return true;
   }
   return false;
}

void KonqFrameHeader::showCloseMenu()
{
    QPopupMenu menu;

    menu.insertItem(i18n("Close"), m_pCloseButton, SLOT(animateClick()));
    menu.exec(QCursor::pos());
}

//###################################################################

KonqFrame::KonqFrame( KonqFrameContainer *_parentContainer, const char *_name )
:QWidget(_parentContainer,_name)
{
   m_pLayout = 0L;
   m_pView = 0L;

   // the frame statusbar
   m_pStatusBar = new KonqFrameStatusBar( this, "KonquerorFrameStatusBar");
   m_pHeader = new KonqFrameHeader(this, "KonquerorFrameHeader");
   connect(m_pStatusBar, SIGNAL(clicked()), this, SLOT(slotStatusBarClicked()));
   connect( m_pStatusBar, SIGNAL( linkedViewClicked( bool ) ), this, SLOT( slotLinkedViewClicked( bool ) ) );
   m_separator = 0;
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
  if (saveURLs)
    config->writeEntry( QString::fromLatin1( "URL" ).prepend( prefix ),
                        childView()->url().url() );
  config->writeEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), childView()->serviceType() );
  config->writeEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), childView()->service()->desktopEntryName() );
  config->writeEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), childView()->isPassiveMode() );
  config->writeEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), childView()->isLinkedView() );
  config->writeEntry( QString::fromLatin1( "ToggleView" ).prepend( prefix ), childView()->isToggleView() );
  config->writeEntry( QString::fromLatin1( "LockedLocation" ).prepend( prefix ), childView()->isLockedLocation() );
}

void KonqFrame::copyHistory( KonqFrameBase *other )
{
    assert( other->frameType() == "View" );
    childView()->copyHistory( static_cast<KonqFrame *>( other )->childView() );
}

KParts::ReadOnlyPart *KonqFrame::attach( const KonqViewFactory &viewFactory )
{
   KonqViewFactory factory( viewFactory );

   // Note that we set the parent to 0.
   // We don't want that deleting the widget deletes the part automatically
   // because we already have that taken care of in KParts...

   m_pPart = factory.create( this, "view widget", 0, "" );

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

   m_pLayout->addWidget( m_pHeader );

   m_pLayout->addWidget( m_pPart->widget() );

   m_pLayout->addWidget( m_pStatusBar );
   m_pPart->widget()->show();
   m_pStatusBar->show();
   m_pHeader->hide();

   m_pLayout->activate();

   m_pPart->widget()->installEventFilter(this);
}

bool KonqFrame::eventFilter(QObject* /*obj*/, QEvent *ev)
{
   if (ev->type()==QEvent::KeyPress)
   {
      QKeyEvent * keyEv = static_cast<QKeyEvent*>(ev);
      if ((keyEv->key()==Key_Tab) && (keyEv->state()==ControlButton))
      {
         emit ((KonqFrameContainer*)parent())->ctrlTabPressed();
         return true;
      };
   };
   return false;
};

void KonqFrame::insertTopWidget( QWidget * widget )
{
    assert(m_pLayout);
    m_pLayout->insertWidget( 0, widget );
    if (widget!=0)
       widget->installEventFilter(this);
}

void KonqFrame::setView( KonqView* child )
{
   m_pView = child;
   if (m_pView)
   {
     connect(m_pView,SIGNAL(sigPartChanged(KonqView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)),
             m_pStatusBar,SLOT(slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)));
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
  if ( !isActivePart() && m_pView && !m_pView->isPassiveMode() )
    m_pView->mainWindow()->viewManager()->setActivePart( part() );
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

void KonqFrame::slotRemoveView()
{
   m_pView->mainWindow()->viewManager()->removeView( m_pView );
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
  int idSecond = id + (int)pow( 2.0, depth );

  //write children sizes
  config->writeEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ), sizes() );

  //write children
  QStringList strlst;
  if( firstChild() )
    strlst.append( QString::fromLatin1( firstChild()->frameType() ) + QString::number(idSecond - 1) );
  if( secondChild() )
    strlst.append( QString::fromLatin1( secondChild()->frameType() ) + QString::number( idSecond ) );

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
    QString newPrefix = QString::fromLatin1( firstChild()->frameType() ) + QString::number(idSecond - 1);
    newPrefix.append( '_' );
    firstChild()->saveConfig( config, newPrefix, saveURLs, id, depth + 1 );
  }

  if( secondChild() ) {
    QString newPrefix = QString::fromLatin1( secondChild()->frameType() ) + QString::number( idSecond );
    newPrefix.append( '_' );
    secondChild()->saveConfig( config, newPrefix, saveURLs, idSecond, depth + 1 );
  }
}

void KonqFrameContainer::copyHistory( KonqFrameBase *other )
{
    assert( other->frameType() == "Container" );
    if ( firstChild() )
        firstChild()->copyHistory( static_cast<KonqFrameContainer *>( other )->firstChild() );
    if ( secondChild() )
        secondChild()->copyHistory( static_cast<KonqFrameContainer *>( other )->secondChild() );
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

void KonqFrameContainer::insertChildFrame( KonqFrameBase* frame )
{
  kdDebug(1202) << "KonqFrameContainer " << this << ": insertChildFrame " << frame << endl;

  if (frame)
  {
      if( !m_pFirstChild )
      {
          m_pFirstChild = frame;
          kdDebug(1202) << "Setting as first child" << endl;
      }
      else if( !m_pSecondChild )
      {
          m_pSecondChild = frame;
          kdDebug(1202) << "Setting as second child" << endl;
      }
      else
        kdWarning(1202) << this << " already has two children..."
                          << m_pFirstChild << " and " << m_pSecondChild << endl;
  } else
    kdWarning(1202) << "KonqFrameContainer " << this << ": insertChildFrame(0L) !" << endl;
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
