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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "konq_events.h"
#include "konq_frame.h"
#include "konq_tabs.h"
#include "konq_view.h"
#include "konq_viewmgr.h"

#include <konq_pixmapprovider.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kstdaccel.h>

#include <qpainter.h>
#include <qtoolbutton.h>
#include <qtabbar.h>
#include <qmenu.h>
#include <qkeysequence.h>
#include <QProgressBar>
//Added by qt3to4:
#include <QPixmap>
#include <QPaintEvent>
#include <QChildEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QMouseEvent>

#include <assert.h>

#define DEFAULT_HEADER_HEIGHT 13

void KonqCheckBox::drawButton( QPainter *p )
{
    //static QPixmap indicator_anchor( UserIcon( "indicator_anchor" ) );
    static QPixmap indicator_connect( UserIcon( "indicator_connect" ) );
    static QPixmap indicator_noconnect( UserIcon( "indicator_noconnect" ) );

   if (isChecked() || isDown())
      p->drawPixmap(0,0,indicator_connect);
   else
      p->drawPixmap(0,0,indicator_noconnect);
}

KonqFrameStatusBar::KonqFrameStatusBar( KonqFrame *_parent )
  : KStatusBar( _parent ),
    m_pParentKonqFrame( _parent )
{
    setSizeGripEnabled( false );

    m_led = new QLabel( this );
    m_led->setAlignment( Qt::AlignCenter );
    m_led->setSizePolicy(QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ));
    addWidget( m_led, 0, false ); // led (active view indicator)
    m_led->hide();

    m_pStatusLabel = new KSqueezedTextLabel( this );
    m_pStatusLabel->setMinimumSize( 0, 0 );
    m_pStatusLabel->setSizePolicy(QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed ));
    m_pStatusLabel->installEventFilter(this);
    addWidget( m_pStatusLabel, 1 /*stretch*/, false ); // status label

    m_pLinkedViewCheckBox = new KonqCheckBox( this );
    m_pLinkedViewCheckBox->setObjectName( "m_pLinkedViewCheckBox" );
    m_pLinkedViewCheckBox->setFocusPolicy(Qt::NoFocus);
    m_pLinkedViewCheckBox->setSizePolicy(QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ));
    m_pLinkedViewCheckBox->setWhatsThis( i18n("Checking this box on at least two views sets those views as 'linked'. "
                          "Then, when you change directories in one view, the other views "
                          "linked with it will automatically update to show the current directory. "
                          "This is especially useful with different types of views, such as a "
                          "directory tree with an icon view or detailed view, and possibly a "
                          "terminal emulator window." ) );
    addWidget( m_pLinkedViewCheckBox, 0, true /*permanent->right align*/ );
    connect( m_pLinkedViewCheckBox, SIGNAL(toggled(bool)),
            this, SIGNAL(linkedViewClicked(bool)) );

    m_progressBar = new QProgressBar( this );
    m_progressBar->setMaximumHeight(fontMetrics().height());
    m_progressBar->hide();
    addWidget( m_progressBar, 0, true /*permanent->right align*/ );

    fontChange(QFont());
    installEventFilter( this );
}

KonqFrameStatusBar::~KonqFrameStatusBar()
{
}

void KonqFrameStatusBar::fontChange(const QFont & /* oldFont */)
{
    int h = fontMetrics().height();
    if ( h < DEFAULT_HEADER_HEIGHT ) h = DEFAULT_HEADER_HEIGHT;
    m_led->setFixedHeight( h + 2 );
    m_progressBar->setFixedHeight( h + 2 );
    // This one is important. Otherwise richtext messages make it grow in height.
    m_pStatusLabel->setFixedHeight( h + 2 );

}

void KonqFrameStatusBar::resizeEvent( QResizeEvent* ev )
{
    //m_progressBar->setGeometry( width()-160, 0, 140, height() );
    //m_pLinkedViewCheckBox->move( width()-15, m_yOffset ); // right justify
    KStatusBar::resizeEvent( ev );
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

   //Blocks menu of custom status bar entries
   //if (event->button()==RightButton)
   //   splitFrameMenu();
}

void KonqFrameStatusBar::splitFrameMenu()
{
   KonqMainWindow * mw = m_pParentKonqFrame->childView()->mainWindow();

   // We have to ship the remove view action ourselves,
   // since this may not be the active view (passive view)
   KAction actRemoveView(i18n("Close View"), "view_remove", 0, m_pParentKonqFrame, SLOT(slotRemoveView()), (KActionCollection*)0, "removethisview");
   //KonqView * nextView = mw->viewManager()->chooseNextView( m_pParentKonqFrame->childView() );
   actRemoveView.setEnabled( mw->mainViewsCount() > 1 || m_pParentKonqFrame->childView()->isToggleView() || m_pParentKonqFrame->childView()->isPassiveMode() );

   // For the rest, we borrow them from the main window
   // ###### might be not right for passive views !
   KActionCollection *actionColl = mw->actionCollection();

   QMenu menu;

   menu.addAction( actionColl->action( "splitviewh" ) );
   menu.addAction( actionColl->action( "splitviewv" ) );
   menu.addSeparator();
   menu.addAction( actionColl->action( "lock" ) );
   menu.addAction( &actRemoveView );

   menu.exec(QCursor::pos());
}

bool KonqFrameStatusBar::eventFilter(QObject* o, QEvent *e)
{
   if (o == m_pStatusLabel && e->type()==QEvent::MouseButtonPress)
   {
      emit clicked();
      update();
      if ( static_cast<QMouseEvent *>(e)->button() == Qt::RightButton)
         splitFrameMenu();
      return true;
   }
   else if ( o == this && e->type() == QEvent::ApplicationPaletteChange )
   {
      unsetPalette();
      updateActiveStatus();
      return true;
   }

   return false;
}

void KonqFrameStatusBar::message( const QString &msg )
{
    // We don't use the message()/clear() mechanism of QStatusBar because
    // it really looks ugly (the label border goes away, the active-view indicator
    // is hidden...)
    QString saveMsg = m_savedMessage;
    slotDisplayStatusText( msg );
    m_savedMessage = saveMsg;
}

void KonqFrameStatusBar::slotDisplayStatusText(const QString& text)
{
    //kDebug(1202)<<"KonqFrameHeader::slotDisplayStatusText("<<text<<")"<<endl;
    //m_pStatusLabel->resize(fontMetrics().width(text),fontMetrics().height()+2);
    m_pStatusLabel->setText(text);
    m_savedMessage = text;
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
}

void KonqFrameStatusBar::slotSpeedProgress( int bytesPerSecond )
{
  QString sizeStr;

  if ( bytesPerSecond > 0 )
    sizeStr = i18n( "%1/s" ,  KIO::convertSize( bytesPerSecond ) );
  else
    sizeStr = i18n( "Stalled" );

  slotDisplayStatusText( sizeStr ); // let's share the same label...
}

void KonqFrameStatusBar::slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *newOne)
{
   if (newOne!=0)
      connect(newOne,SIGNAL(setStatusBarText(const QString &)),this,SLOT(slotDisplayStatusText(const QString&)));
   slotDisplayStatusText( QString() );
}

void KonqFrameStatusBar::showActiveViewIndicator( bool b )
{
    m_led->setShown( b );
    updateActiveStatus();
}

void KonqFrameStatusBar::showLinkedViewIndicator( bool b )
{
    m_pLinkedViewCheckBox->setShown( b );
}

void KonqFrameStatusBar::setLinkedView( bool b )
{
    m_pLinkedViewCheckBox->blockSignals( true );
    m_pLinkedViewCheckBox->setChecked( b );
    m_pLinkedViewCheckBox->blockSignals( false );
}

void KonqFrameStatusBar::updateActiveStatus()
{
    if ( m_led->isHidden() )
    {
        unsetPalette();
        return;
    }

    bool hasFocus = m_pParentKonqFrame->isActivePart();

    const QColorGroup& activeCg = kapp->palette().active();
    setPaletteBackgroundColor( hasFocus ? activeCg.midlight() : activeCg.mid() );

    static QPixmap indicator_viewactive( UserIcon( "indicator_viewactive" ) );
    static QPixmap indicator_empty( UserIcon( "indicator_empty" ) );
    m_led->setPixmap( hasFocus ? indicator_viewactive : indicator_empty );
}

//###################################################################

void KonqFrameBase::printFrameInfo(const QString& spaces)
{
    kDebug(1202) << spaces << "KonqFrameBase " << this << " printFrameInfo not implemented in derived class!" << endl;
}

//###################################################################

KonqFrame::KonqFrame( QWidget* parent, KonqFrameContainerBase *parentContainer, const char *name )
    : QWidget (parent, name )
{
   //kDebug(1202) << "KonqFrame::KonqFrame()" << endl;

   m_pLayout = 0L;
   m_pView = 0L;

   // the frame statusbar
   m_pStatusBar = new KonqFrameStatusBar( this);
   m_pStatusBar->setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
   connect(m_pStatusBar, SIGNAL(clicked()), this, SLOT(slotStatusBarClicked()));
   connect( m_pStatusBar, SIGNAL( linkedViewClicked( bool ) ), this, SLOT( slotLinkedViewClicked( bool ) ) );
   m_separator = 0;
   m_pParentContainer = parentContainer;
}

KonqFrame::~KonqFrame()
{
   //kDebug(1202) << "KonqFrame::~KonqFrame() " << this << endl;
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
    config->writePathEntry( QString::fromLatin1( "URL" ).prepend( prefix ),
                        childView()->url().url() );
  config->writeEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), childView()->serviceType() );
  config->writeEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), childView()->service()->desktopEntryName() );
  config->writeEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), childView()->isPassiveMode() );
  config->writeEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), childView()->isLinkedView() );
  config->writeEntry( QString::fromLatin1( "ToggleView" ).prepend( prefix ), childView()->isToggleView() );
  config->writeEntry( QString::fromLatin1( "LockedLocation" ).prepend( prefix ), childView()->isLockedLocation() );
  //config->writeEntry( QString::fromLatin1( "ShowStatusBar" ).prepend( prefix ), statusbar()->isVisible() );
  if (this == docContainer) config->writeEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), true );

  KonqConfigEvent ev( config, prefix+"_", true/*save*/);
  QApplication::sendEvent( childView()->part(), &ev );
}

void KonqFrame::copyHistory( KonqFrameBase *other )
{
    assert( other->frameType() == "View" );
    childView()->copyHistory( static_cast<KonqFrame *>( other )->childView() );
}

void KonqFrame::printFrameInfo( const QString& spaces )
{
   QString className = "NoPart";
   if (part()) className = part()->widget()->metaObject()->className();
   kDebug(1202) << spaces << "KonqFrame " << this << " visible=" << QString("%1").arg(isVisible()) << " containing view "
                 << childView() << " visible=" << QString("%1").arg(isVisible())
                 << " and part " << part() << " whose widget is a " << className << endl;
}

KParts::ReadOnlyPart *KonqFrame::attach( const KonqViewFactory &viewFactory )
{
   KonqViewFactory factory( viewFactory );

   // Note that we set the parent to 0.
   // We don't want that deleting the widget deletes the part automatically
   // because we already have that taken care of in KParts...

   m_pPart = factory.create( this, 0 );

   assert( m_pPart->widget() );

   attachInternal();

   m_pStatusBar->slotConnectToNewView(0, 0,m_pPart);

   return m_pPart;
}

void KonqFrame::attachInternal()
{
   //kDebug(1202) << "KonqFrame::attachInternal()" << endl;
   delete m_pLayout;

   m_pLayout = new QVBoxLayout( this );
   m_pLayout->setObjectName( "KonqFrame's QVBoxLayout" );
   m_pLayout->setMargin( 0 );
   m_pLayout->setSpacing( 0 );

   m_pLayout->addWidget( m_pPart->widget(), 1 );

   m_pLayout->addWidget( m_pStatusBar, 0 );
   m_pPart->widget()->show();

   m_pLayout->activate();

   m_pPart->widget()->installEventFilter(this);
}

bool KonqFrame::eventFilter(QObject* /*obj*/, QEvent *ev)
{
   if (ev->type()==QEvent::KeyPress)
   {
      QKeyEvent * keyEv = static_cast<QKeyEvent*>(ev);
      if ((keyEv->key()==Qt::Key_Tab) && (keyEv->modifiers()==Qt::ControlModifier))
      {
         emit ((KonqFrameContainer*)parent())->ctrlTabPressed();
         return true;
      }
   }
   return false;
}

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
}

void KonqFrame::setTitle( const QString &title , QWidget* /*sender*/)
{
  //kDebug(1202) << "KonqFrame::setTitle( " << title << " )" << endl;
  m_title = title;
  if (m_pParentContainer) m_pParentContainer->setTitle( title , this);
}

void KonqFrame::setTabIcon( const KUrl &url, QWidget* /*sender*/ )
{
  //kDebug(1202) << "KonqFrame::setTabIcon( " << url << " )" << endl;
  if (m_pParentContainer) m_pParentContainer->setTabIcon( url, this );
}

void KonqFrame::reparentFrame( QWidget* parent, const QPoint & p )
{
    setParent( parent );
    move( p );
}

void KonqFrame::slotStatusBarClicked()
{
  if ( !isActivePart() && m_pView && !m_pView->isPassiveMode() )
    m_pView->mainWindow()->viewManager()->setActivePart( part() );
}

void KonqFrame::slotLinkedViewClicked( bool mode )
{
  if ( m_pView->mainWindow()->linkableViewsCount() == 2 )
    m_pView->mainWindow()->slotLinkView();
  else
    m_pView->setLinkedView( mode );
}

void
KonqFrame::paintEvent( QPaintEvent* )
{
#ifdef __GNUC__
    #warning "m_pStatusBar->repaint() leads to endless recursion; does anyone know why it's needed?"
#endif
//   m_pStatusBar->repaint();
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

KonqView* KonqFrame::childView() const
{
  return m_pView;
}

KonqView* KonqFrame::activeChildView()
{
  return m_pView;
}

#include "konq_frame.moc"
