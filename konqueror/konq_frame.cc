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
#include <qtabbar.h>
#include <qptrlist.h>
#include <qpopupmenu.h>
#include <qkeysequence.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kprogress.h>
#include <klocale.h>

#include "konq_frame.h"
#include "konq_view.h"
#include "konq_viewmgr.h"

#include <konq_pixmapprovider.h>
#include <kstdaccel.h>
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

   m_progressBar = new KProgress( this );
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
   KAction actRemoveView(i18n("Remove View"), 0, m_pParentKonqFrame, SLOT(slotRemoveView()), (QObject*)0, "removethisview");
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
   m_pStatusLabel->resize(fontMetrics().width(text),fontMetrics().height()+2);
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
    sizeStr = i18n( "%1/s" ).arg( KIO::convertSize( bytesPerSecond ) );
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

   QBrush bg = kapp->palette().active().brush ( QColorGroup::Background );
   pal.setBrush( QColorGroup::Background,
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

void KonqFrameBase::printFrameInfo(QString spaces)
{
	kdDebug(1202) << spaces << "KonqFrameBase " << this << ", this shouldn't happen!" << endl;
}

//###################################################################

KonqFrame::KonqFrame( QWidget* parent, KonqFrameContainerBase *parentContainer, const char *name )
:QWidget(parent,name)
{
   //kdDebug(1202) << "KonqFrame::KonqFrame()" << endl;

   m_pLayout = 0L;
   m_pView = 0L;

   // the frame statusbar
   m_pStatusBar = new KonqFrameStatusBar( this, "KonquerorFrameStatusBar");
   m_pHeader = new KonqFrameHeader(this, "KonquerorFrameHeader");
   connect(m_pStatusBar, SIGNAL(clicked()), this, SLOT(slotStatusBarClicked()));
   connect( m_pStatusBar, SIGNAL( linkedViewClicked( bool ) ), this, SLOT( slotLinkedViewClicked( bool ) ) );
   m_separator = 0;
   m_pParentContainer = parentContainer;
}

KonqFrame::~KonqFrame()
{
   //kdDebug(1202) << "KonqFrame::~KonqFrame() " << this << endl;
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

void KonqFrame::saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int /*id*/, int /*depth*/ )
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
  //config->writeEntry( QString::fromLatin1( "ShowStatusBar" ).prepend( prefix ), statusbar()->isVisible() );
  if (this == docContainer) config->writeEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), true );
}

void KonqFrame::copyHistory( KonqFrameBase *other )
{
    assert( other->frameType() == "View" );
    childView()->copyHistory( static_cast<KonqFrame *>( other )->childView() );
}

void KonqFrame::printFrameInfo( QString spaces )
{
   QString className = "NoPart";
   if (part()) className = part()->widget()->className();
   kdDebug(1202) << spaces << "KonqFrame " << this << " visible=" << QString("%1").arg(isVisible()) << " containing view "
                 << childView() << " visible=" << QString("%1").arg(isVisible())
                 << " and part " << part() << " whose widget is a " << className << endl;
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
   //kdDebug(1202) << "KonqFrame::attachInternal()" << endl;
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

void KonqFrame::setTitle( QString title , QWidget* /*sender*/)
{
  //kdDebug(1202) << "KonqFrame::setTitle( " << title << " )" << endl;
  if (m_pParentContainer) m_pParentContainer->setTitle( title , this);
}

void KonqFrame::setTabIcon( QString url, QWidget* /*sender*/ )
{
  //kdDebug(1202) << "KonqFrame::setTabIcon( " << url << " )" << endl;
  if (m_pParentContainer) m_pParentContainer->setTabIcon( url, this );
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
  bool oneFollowActive=false;
  if (m_pView->mainWindow()->viewCount() == 2)
  {
    // Two views : link both
    KonqMainWindow::MapViews mapViews = m_pView->mainWindow()->viewMap();
    KonqMainWindow::MapViews::Iterator it = mapViews.begin();
    oneFollowActive=(*it)->isFollowActive();
    ++it;
    oneFollowActive |=(*it)->isFollowActive();
    if (oneFollowActive) mode=false;

    it=mapViews.begin();
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

void KonqFrame::activateChild()
{
  if (m_pView && !m_pView->isPassiveMode() )
    m_pView->mainWindow()->viewManager()->setActivePart( part() );
}

//###################################################################

void KonqFrameContainerBase::printFrameInfo(QString spaces)
{
	kdDebug(1202) << spaces << "KonqFrameContainerBase " << this << ", this shouldn't happen!" << endl;
}

//###################################################################

KonqFrameContainer::KonqFrameContainer( Orientation o,
                                        QWidget* parent,
                                        KonqFrameContainerBase* parentContainer,
                                        const char * name)
  : QSplitter( o, parent, name )
{
  m_pParentContainer = parentContainer;
  m_pFirstChild = 0L;
  m_pSecondChild = 0L;
  m_pActiveChild = 0L;
  setOpaqueResize( true );
}

KonqFrameContainer::~KonqFrameContainer()
{
    //kdDebug(1202) << "KonqFrameContainer::~KonqFrameContainer() " << this << " - " << className() << endl;
		if ( m_pFirstChild )
			delete m_pFirstChild;
		if ( m_pSecondChild )
			delete m_pSecondChild;
}

void KonqFrameContainer::listViews( ChildViewList *viewList )
{
   if( m_pFirstChild )
      m_pFirstChild->listViews( viewList );

   if( m_pSecondChild )
      m_pSecondChild->listViews( viewList );
}

void KonqFrameContainer::saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id, int depth )
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

  //write docContainer
  if (this == docContainer) config->writeEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), true );

  if (m_pSecondChild == m_pActiveChild) config->writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ), 1 );
  else config->writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ), 0 );

  //write child configs
  if( firstChild() ) {
    QString newPrefix = QString::fromLatin1( firstChild()->frameType() ) + QString::number(idSecond - 1);
    newPrefix.append( '_' );
    firstChild()->saveConfig( config, newPrefix, saveURLs, docContainer, id, depth + 1 );
  }

  if( secondChild() ) {
    QString newPrefix = QString::fromLatin1( secondChild()->frameType() ) + QString::number( idSecond );
    newPrefix.append( '_' );
    secondChild()->saveConfig( config, newPrefix, saveURLs, docContainer, idSecond, depth + 1 );
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

void KonqFrameContainer::printFrameInfo( QString spaces )
{
        kdDebug(1202) << spaces << "KonqFrameContainer " << this << " visible=" << QString("%1").arg(isVisible())
                      << " activeChild=" << m_pActiveChild << endl;
        if (!m_pActiveChild)
            kdDebug(1202) << "WARNING: " << this << " has a null active child!" << endl;
        KonqFrameBase* child = firstChild();
        if (child != 0L)
            child->printFrameInfo(spaces + "  ");
        else
            kdDebug(1202) << spaces << "  Null child" << endl;
        child = secondChild();
        if (child != 0L)
            child->printFrameInfo(spaces + "  ");
        else
            kdDebug(1202) << spaces << "  Null child" << endl;
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

void KonqFrameContainer::setTitle( QString title , QWidget* sender)
{
  //kdDebug(1202) << "KonqFrameContainer::setTitle( " << title << " , " << sender << " )" << endl;
  if (m_pParentContainer && activeChild() && (sender == activeChild()->widget())) m_pParentContainer->setTitle( title , this);
}

void KonqFrameContainer::setTabIcon( QString url, QWidget* sender )
{
  //kdDebug(1202) << "KonqFrameContainer::setTabIcon( " << url << " , " << sender << " )" << endl;
  if (m_pParentContainer && activeChild() && (sender == activeChild()->widget())) m_pParentContainer->setTabIcon( url, this );
}

void KonqFrameContainer::insertChildFrame( KonqFrameBase* frame, int /*index*/  )
{
  //kdDebug(1202) << "KonqFrameContainer " << this << ": insertChildFrame " << frame << endl;

  if (frame)
  {
      if( !m_pFirstChild )
      {
          m_pFirstChild = frame;
          frame->setParentContainer(this);
          //kdDebug(1202) << "Setting as first child" << endl;
      }
      else if( !m_pSecondChild )
      {
          m_pSecondChild = frame;
          frame->setParentContainer(this);
          //kdDebug(1202) << "Setting as second child" << endl;
      }
      else
        kdWarning(1202) << this << " already has two children..."
                        << m_pFirstChild << " and " << m_pSecondChild << endl;
  } else
    kdWarning(1202) << "KonqFrameContainer " << this << ": insertChildFrame(0L) !" << endl;
}

void KonqFrameContainer::removeChildFrame( KonqFrameBase * frame )
{
  //kdDebug(1202) << "KonqFrameContainer::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;

  if( m_pFirstChild == frame )
  {
    m_pFirstChild = m_pSecondChild;
    m_pSecondChild = 0L;
  }
  else if( m_pSecondChild == frame )
    m_pSecondChild = 0L;

  else
    kdWarning(1202) << this << " Can't find this child:" << frame << endl;
}

//###################################################################

// I'd like to thank the good people at QTella for the inspiration for KonqTabBar

KonqTabBar::KonqTabBar(KonqViewManager* viewManager, KonqFrameTabs *parent, const char *name)
  : QTabBar(parent, name)
{
    m_pTabWidget = parent;
    m_pViewManager = viewManager;

    m_pPopupMenu = new QPopupMenu( this );

    m_pPopupMenu->insertItem( SmallIcon( "tab_new" ), i18n("&New Tab"), m_pViewManager->mainWindow(), SLOT( slotAddTab() ), QKeySequence("Ctrl+Shift+N") );
    m_pPopupMenu->insertItem( SmallIcon( "tab_duplicate" ), i18n("&Duplicate Tab"), m_pViewManager->mainWindow(), SLOT( slotDuplicateTabPopup() ), QKeySequence("Ctrl+Shift+D") );
    m_pPopupMenu->insertSeparator();
    m_pPopupMenu->insertItem( SmallIcon( "tab_breakoff" ), i18n("D&etach Tab"), m_pViewManager->mainWindow(), SLOT( slotBreakOffTabPopup() ), QKeySequence("Ctrl+Shift+B") );
    m_pPopupMenu->insertItem( SmallIcon( "tab_remove" ), i18n("&Close Tab"), m_pViewManager->mainWindow(), SLOT( slotRemoveTabPopup() ), QKeySequence("Ctrl+W") );
    m_pPopupMenu->insertSeparator();
    m_pPopupMenu->insertItem( SmallIcon( "reload" ), i18n( "&Reload" ), m_pViewManager->mainWindow(), SLOT( slotReload() ), KStdAccel::key(KStdAccel::Reload) );
    m_pPopupMenu->insertItem( SmallIcon( "reload_all_tab" ), i18n( "&Reload All Tab" ), m_pViewManager->mainWindow(), SLOT( slotReloadAllTab() ));
    m_pPopupMenu->insertSeparator();
    m_pPopupMenu->insertItem( SmallIcon( "tab_remove" ), i18n("Close &Other Tabs"), m_pViewManager->mainWindow(), SLOT( slotRemoveOtherTabsPopup() ) );
}

KonqTabBar::~KonqTabBar()
{
}

void KonqTabBar::mousePressEvent(QMouseEvent *e)
{
  //kdDebug(1202) << "KonqTabBar::mousePressEvent begin" << endl;

  if (e->button() == RightButton)
  {
    QTab *tab = selectTab( e->pos() );
    if ( tab == 0L ) return;
    QWidget* page = m_pTabWidget->page( indexOf( tab->identifier() ) );
    if (page == 0L) return;
    m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(page) );
    m_pPopupMenu->exec( mapToGlobal( e->pos() ) );
  }

  QTabBar::mousePressEvent( e );

  //kdDebug(1202) << "KonqTabBar::mousePressEvent end" << endl;
}

KonqFrameTabs::KonqFrameTabs(QWidget* parent, KonqFrameContainerBase* parentContainer, KonqViewManager* viewManager, const char * name)
  : QTabWidget(parent, name)
{
  //kdDebug(1202) << "KonqFrameTabs::KonqFrameTabs()" << endl;

  m_pParentContainer = parentContainer;
  m_pChildFrameList = new QPtrList<KonqFrameBase>;
  m_pChildFrameList->setAutoDelete(false);
  m_pActiveChild = 0L;
  m_pViewManager = viewManager;

  setTabBar( new KonqTabBar( m_pViewManager, this ) );

  connect( this, SIGNAL( currentChanged ( QWidget * ) ), this, SLOT( slotCurrentChanged( QWidget* ) ) );
}

KonqFrameTabs::~KonqFrameTabs()
{
  //kdDebug(1202) << "KonqFrameTabs::~KonqFrameTabs() " << this << " - " << className() << endl;
  m_pChildFrameList->setAutoDelete(true);
  delete m_pChildFrameList;
}

void KonqFrameTabs::listViews( ChildViewList *viewList ) {
  int childFrameCount = m_pChildFrameList->count();
  for( int i=0 ; i<childFrameCount ; i++) m_pChildFrameList->at(i)->listViews(viewList);
}

void KonqFrameTabs::saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id, int depth )
{
  //write children
  QStringList strlst;
  int i = 0;
  QString newPrefix;
  for (KonqFrameBase* it = m_pChildFrameList->first(); it; it = m_pChildFrameList->next())
  {
    newPrefix = QString::fromLatin1( it->frameType() ) + "T" + QString::number(i);
    strlst.append( newPrefix );
    newPrefix.append( '_' );
    it->saveConfig( config, newPrefix, saveURLs, docContainer, id, depth);
    i++;
  }

  config->writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  config->writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ), currentPageIndex() );
}

void KonqFrameTabs::copyHistory( KonqFrameBase *other )
{
  if( other->frameType() != "Tabs" ) {
    kdDebug(1202) << "Frame types are not the same" << endl;
    return;
  }

  for (uint i = 0; i < m_pChildFrameList->count(); i++ )
  {
    m_pChildFrameList->at(i)->copyHistory( static_cast<KonqFrameTabs *>( other )->m_pChildFrameList->at(i) );
  }
}

void KonqFrameTabs::printFrameInfo( QString spaces )
{
  kdDebug(1202) << spaces << "KonqFrameTabs " << this << " visible=" << QString("%1").arg(isVisible())
                << " activeChild=" << m_pActiveChild << endl;
  if (!m_pActiveChild) kdDebug(1202) << "WARNING: " << this << " has a null active child!" << endl;
  KonqFrameBase* child;
  int childFrameCount = m_pChildFrameList->count();
  for (int i = 0 ; i < childFrameCount ; i++) {
    child = m_pChildFrameList->at(i);
    if (child != 0L) child->printFrameInfo(spaces + "  ");
    else kdDebug(1202) << spaces << "  Null child" << endl;
  }
}

void KonqFrameTabs::reparentFrame( QWidget* parent, const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, p, showIt );
}

void KonqFrameTabs::setTitle( QString title , QWidget* sender)
{
  // kdDebug(1202) << "KonqFrameTabs::setTitle( " << title << " , " << sender << " )" << endl;
  QString newTitle = title;
  newTitle.replace('&', "&&");

  removeTabToolTip( sender );
  if (newTitle.length() > 30)
  {
    setTabToolTip( sender, newTitle );
    newTitle = newTitle.left(27) + "...";
  }
  if (tabLabel( sender ) != newTitle)
    changeTab( sender , newTitle );
}

void KonqFrameTabs::setTabIcon( QString url, QWidget* sender )
{
  //kdDebug(1202) << "KonqFrameTabs::setTabIcon( " << url << " , " << sender << " )" << endl;
  QIconSet iconSet = QIconSet( KonqPixmapProvider::self()->pixmapFor( url ) );
  if (tabIconSet( sender ).pixmap().serialNumber() != iconSet.pixmap().serialNumber())
    setTabIconSet( sender, iconSet );
}

void KonqFrameTabs::activateChild()
{
  if (m_pActiveChild)
  {
    showPage( m_pActiveChild->widget() );
    m_pActiveChild->activateChild();
  }
}

void KonqFrameTabs::insertChildFrame( KonqFrameBase* frame, int index )
{
  //kdDebug(1202) << "KonqFrameTabs " << this << ": insertChildFrame " << frame << endl;

  if (frame)
  {
    //kdDebug(1202) << "Adding frame" << endl;
    insertTab(frame->widget(),"", index);
    frame->setParentContainer(this);
    if (index == -1) m_pChildFrameList->append(frame);
    else m_pChildFrameList->insert(index, frame);
    KonqView* activeChildView = frame->activeChildView();
    if (activeChildView != 0L) {
      activeChildView->setCaption( activeChildView->caption() );
      activeChildView->setTabIcon( activeChildView->url().url() );
    }
  }
  else
    kdWarning(1202) << "KonqFrameTabs " << this << ": insertChildFrame(0L) !" << endl;
}

void KonqFrameTabs::removeChildFrame( KonqFrameBase * frame )
{
  //kdDebug(1202) << "KonqFrameTabs::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;
  if (frame) {
    removePage(frame->widget());
    m_pChildFrameList->remove(frame);
  }
  else
    kdWarning(1202) << "KonqFrameTabs " << this << ": removeChildFrame(0L) !" << endl;

  //kdDebug(1202) << "KonqFrameTabs::RemoveChildFrame finished" << endl;
}

void KonqFrameTabs::slotCurrentChanged( QWidget* newPage )
{
  KonqFrameBase* currentFrame = dynamic_cast<KonqFrameBase*>(newPage);

  if (!m_pViewManager->isLoadingProfile()) {
    m_pActiveChild = currentFrame;
    currentFrame->activateChild();
  }
}

void KonqFrameTabs::moveTabLeft(int index)
{
    if ( index == 0 )
        return;
    KonqFrameBase* currentFrame = m_pChildFrameList->at(index );
    kdDebug()<<" currentFrame :"<<currentFrame<<" index :"<<index<<endl;
    removePage(currentFrame->widget());
    m_pChildFrameList->remove(currentFrame);

    insertChildFrame( currentFrame, index-1 );
    setCurrentPage( index-1 );
}

void KonqFrameTabs::moveTabRight(int index)
{
    if ( index == count()-1 )
        return;
    KonqFrameBase* currentFrame = m_pChildFrameList->at(index );
    kdDebug()<<" currentFrame :"<<currentFrame<<" index :"<<index<<endl;
    removePage(currentFrame->widget());
    m_pChildFrameList->remove(currentFrame);

    insertChildFrame( currentFrame, index+1 );
    setCurrentPage( index+1 );
}


#include "konq_frame.moc"
