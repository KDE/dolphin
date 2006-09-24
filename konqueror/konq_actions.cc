/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_actions.h"

#include <assert.h>

#include <ktoolbar.h>
#include <kanimwidget.h>
#include <kdebug.h>
#include <kstringhandler.h>

#include <konq_pixmapprovider.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kmenu.h>
#include <kapplication.h>
#include <ktoolbar.h>

#include "konq_view.h"
#include "konq_settingsxt.h"
#include <kauthorized.h>

#include <algorithm>
#include <QHBoxLayout>

template class QList<KonqHistoryEntry*>;

/////////////////

//static - used by KonqHistoryAction and KonqBidiHistoryAction
void KonqBidiHistoryAction::fillHistoryPopup( const QList<HistoryEntry*> &history, int historyIndex,
                                          QMenu * popup,
                                          bool onlyBack,
                                          bool onlyForward,
                                          bool checkCurrentItem,
                                          int startPos )
{
  assert ( popup ); // kill me if this 0... :/

  //kDebug(1202) << "fillHistoryPopup position: " << history.at() << endl;
  HistoryEntry * current = history[ historyIndex ];
  int index = 0;
  if (onlyBack || onlyForward)
  {
      index += historyIndex; // Jump to current item
      if ( !onlyForward ) --index; else ++index; // And move off it
  } else if ( startPos )
      index += startPos; // Jump to specified start pos

  uint i = 0;
  while ( index < history.count() && index >= 0 )
  {
      QString text = history[ index ]->title;
      text = KStringHandler::cEmSqueeze(text, popup->fontMetrics(), 30); //CT: squeeze
      text.replace( "&", "&&" );
      if ( checkCurrentItem && history[ index ] == current )
      {
          int id = popup->insertItem( text ); // no pixmap if checked
          popup->setItemChecked( id, true );
      } else
          popup->insertItem( QIcon( KonqPixmapProvider::self()->pixmapFor(
                                        history[ index ]->url.url() ) ), text );
      if ( ++i > 10 )
          break;
      if ( !onlyForward ) --index; else ++index;
  }
  //kDebug(1202) << "After fillHistoryPopup position: " << history.at() << endl;
}

///////////////////////////////

KonqBidiHistoryAction::KonqBidiHistoryAction ( const QString & text, KActionCollection* parent, const char* name )
  : KAction( text, parent, name )
{
  setShortcutConfigurable(false);
  m_firstIndex = 0;
  setMenu(new KMenu);

  connect( menu(), SIGNAL( aboutToShow() ), SIGNAL( menuAboutToShow() ) );
  connect( menu(), SIGNAL( triggered( QAction* ) ), SLOT( slotTriggered( QAction* ) ) );
}

KonqBidiHistoryAction::~ KonqBidiHistoryAction( )
{
  delete menu();
}

QWidget * KonqBidiHistoryAction::createToolBarWidget( QToolBar * parent )
{
  QToolButton* button = new QToolButton(parent);
  button->setAutoRaise(true);
  button->setFocusPolicy(Qt::NoFocus);
  button->setIconSize(parent->iconSize());
  button->setToolButtonStyle(parent->toolButtonStyle());
  QObject::connect(parent, SIGNAL(iconSizeChanged(const QSize&)),
                   button, SLOT(setIconSize(const QSize&)));
  QObject::connect(parent, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
                   button, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));
  button->setDefaultAction(this);
  QObject::connect(button, SIGNAL(triggered(QAction*)), parent, SIGNAL(actionTriggered(QAction*)));

  button->setPopupMode(QToolButton::MenuButtonPopup);

  m_firstIndex = menu()->actions().count();

  return button;
}

void KonqBidiHistoryAction::fillGoMenu( const QList<HistoryEntry*> & history, int historyIndex )
{
    if (history.isEmpty())
        return; // nothing to do

    //kDebug(1202) << "fillGoMenu position: " << history.at() << endl;
    if ( m_firstIndex == 0 ) // should never happen since done in plug
        m_firstIndex = menu()->actions().count();
    else
    { // Clean up old history (from the end, to avoid shifts)
        for ( int i = menu()->actions().count()-1 ; i >= m_firstIndex; i-- )
            menu()->removeAction( menu()->actions()[i] );
    }
    // TODO perhaps smarter algorithm (rename existing items, create new ones only if not enough) ?

    // Ok, we want to show 10 items in all, among which the current url...

    if ( history.count() <= 9 )
    {
        // First case: limited history in both directions -> show it all
        m_startPos = history.count() - 1; // Start right from the end
    } else
    // Second case: big history, in one or both directions
    {
        // Assume both directions first (in this case we place the current URL in the middle)
        m_startPos = historyIndex + 4;

        // Forward not big enough ?
        if ( historyIndex > history.count() - 4 )
          m_startPos = history.count() - 1;
    }
    Q_ASSERT( m_startPos >= 0 && m_startPos < history.count() );
    if ( m_startPos < 0 || m_startPos >= history.count() )
    {
        kWarning() << "m_startPos=" << m_startPos << " history.count()=" << history.count() << endl;
        return;
    }
    m_currentPos = historyIndex; // for slotActivated
    KonqBidiHistoryAction::fillHistoryPopup( history, historyIndex, menu(), false, false, true, m_startPos );
}

void KonqBidiHistoryAction::slotTriggered( QAction* action )
{
  // 1 for first item in the list, etc.
  int index = menu()->actions().indexOf(action) - m_firstIndex + 1;
  if ( index > 0 )
  {
      kDebug(1202) << "Item clicked has index " << index << endl;
      // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
      int steps = ( m_startPos+1 ) - index - m_currentPos; // make a drawing to understand this :-)
      kDebug(1202) << "Emit activated with steps = " << steps << endl;
      emit step( steps );
  }
}

///////////////////////////////

KonqLogoAction::KonqLogoAction( const QString& text, const KShortcut& accel, KActionCollection* parent, const char* name )
  : KAction( text, parent, name )
{
  setShortcut(accel);
  //setToolBarWidgetFactory(this);
}

KonqLogoAction::KonqLogoAction( const QString& text, const KShortcut& accel,
                               QObject* receiver, const char* slot, KActionCollection* parent, const char* name )
  : KAction( text, parent, name )
{
  setShortcut(accel);
  connect(this, SIGNAL(triggered(bool)), receiver, slot);
  //setToolBarWidgetFactory(this);
}

KonqLogoAction::KonqLogoAction( const QString& text, const KIcon& pix, const KShortcut&  accel, KActionCollection* parent, const char* name )
  : KAction( text, parent, name )
{
  setIcon(pix);
  setShortcut(accel);
  //setToolBarWidgetFactory(this);
}

KonqLogoAction::KonqLogoAction( const QString& text, const KIcon& pix, const KShortcut& accel, QObject* receiver, const char* slot, KActionCollection* parent, const char* name )
  : KAction( text, parent, name )
{
  setIcon(pix);
  setShortcut(accel);
  connect(this, SIGNAL(triggered(bool)), receiver, slot);
  //setToolBarWidgetFactory(this);
}

KonqLogoAction::KonqLogoAction( const QStringList& icons, QObject* receiver,
                                const char* slot, KActionCollection* parent,
                                const char* name )
    : KAction( QString(), parent, name ) // text missing !
{
  iconList = icons;
  connect(this, SIGNAL(toggled(bool)), receiver, slot);
  //setToolBarWidgetFactory(this);
}

void KonqLogoAction::start()
{
  emit startAnimation();
}

void KonqLogoAction::stop()
{
  emit stopAnimation();
}


QWidget * KonqLogoAction::createToolBarWidget( QToolBar * parent )
{
  QWidget* container = new QWidget(parent);

  QHBoxLayout* layout = new QHBoxLayout(container);
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addStretch();

  KAnimatedButton *button = new KAnimatedButton(container);
  button->setAutoRaise(true);
  button->setFocusPolicy(Qt::NoFocus);
  button->setIconSize(parent->iconSize());
  button->setToolButtonStyle(Qt::ToolButtonIconOnly);
  connect(parent, SIGNAL(iconSizeChanged(const QSize&)), button, SLOT(setIconSize(const QSize&)));
  connect(parent, SIGNAL(iconSizeChanged(const QSize&)), button, SLOT(updateIcons()));
  connect(button, SIGNAL(triggered(QAction*)), parent, SIGNAL(actionTriggered(QAction*)));
  button->setDefaultAction(this);

  connect(this, SIGNAL(startAnimation()), button, SLOT(start()));
  connect(this, SIGNAL(stopAnimation()), button, SLOT(stop()));

  button->setIcons(QString("kde"));

  layout->addWidget(button);

  return container;
}

///////////

KonqViewModeAction::KonqViewModeAction( const QString& desktopEntryName,
                                        const QString &text, const KIcon &icon,
                                        KActionCollection *parent, const char *name )
    : KAction( icon, text, parent, name ),
      m_desktopEntryName( desktopEntryName )
{
    setMenu(new QMenu());
    //setToolBarWidgetFactory(this);

    /*connect( m_menu, SIGNAL( aboutToShow() ),
             this, SLOT( slotPopupAboutToShow() ) );
    connect( m_menu, SIGNAL( activated( int ) ),
             this, SLOT( slotPopupActivated() ) );
    connect( m_menu, SIGNAL( aboutToHide() ),
             this, SLOT( slotPopupAboutToHide() ) );*/
}

KonqViewModeAction::~KonqViewModeAction()
{
    delete menu();
}

QWidget * KonqViewModeAction::createToolBarWidget( QToolBar * parent )
{
  QToolButton* button = new QToolButton(parent);
  button->setAutoRaise(true);
  button->setFocusPolicy(Qt::NoFocus);
  button->setIconSize(parent->iconSize());
  button->setToolButtonStyle(parent->toolButtonStyle());
  QObject::connect(parent, SIGNAL(iconSizeChanged(const QSize&)),
                   button, SLOT(setIconSize(const QSize&)));
  QObject::connect(parent, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
                   button, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));
  button->setDefaultAction(this);
  QObject::connect(button, SIGNAL(triggered(QAction*)), parent, SIGNAL(actionTriggered(QAction*)));

  //if (menu()->count() > 1)
    button->setPopupMode(QToolButton::InstantPopup);

  return button;
}

/*
void KonqViewModeAction::slotPopupAboutToShow()
{
    m_popupActivated = false;
}

void KonqViewModeAction::slotPopupActivated()
{
    m_popupActivated = true;
}

void KonqViewModeAction::slotPopupAboutToHide()
{
    if ( !m_popupActivated )
    {
        int i = 0;
        for (; i < containerCount(); ++i )
        {
            QWidget *widget = container( i );
            if ( !widget->inherits( "KToolBar" ) )
                continue;

            KToolBar *tb = static_cast<KToolBar *>( widget );

            KToolBarButton *button = tb->getButton( itemId( i ) );

            button->setDown( isChecked() );
        }
    }
}*/


KonqHistoryList* KonqMostOftenURLSAction::s_mostEntries = 0;
uint KonqMostOftenURLSAction::s_maxEntries = 0;

KonqMostOftenURLSAction::KonqMostOftenURLSAction( const QString& text,
						  KActionCollection *parent,
						  const char *name )
    : KActionMenu( KIcon("goto"), text, parent, name )
{
    setDelayed( false );

    connect( menu(), SIGNAL( aboutToShow() ), SLOT( slotFillMenu() ));
    //connect( popupMenu(), SIGNAL( aboutToHide() ), SLOT( slotClearMenu() ));
    connect( menu(), SIGNAL( activated( int ) ),
	     SLOT( slotActivated(int) ));
    // Need to do all this upfront for a correct initial state
    init();
}

KonqMostOftenURLSAction::~KonqMostOftenURLSAction()
{
}

void KonqMostOftenURLSAction::init()
{
    s_maxEntries = KonqSettings::numberofmostvisitedURLs();

    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    setEnabled( !mgr->entries().isEmpty() && s_maxEntries > 0 );
}

void KonqMostOftenURLSAction::inSort( const KonqHistoryEntry& entry ) {
    KonqHistoryList::iterator it = std::lower_bound( s_mostEntries->begin(),
                                                     s_mostEntries->end(),
                                                     entry,
                                                     numberOfVisitOrder );
    s_mostEntries->insert( it, entry );
}

void KonqMostOftenURLSAction::parseHistory() // only ever called once
{
    KonqHistoryManager *mgr = KonqHistoryManager::kself();

    connect( mgr, SIGNAL( entryAdded( const KonqHistoryEntry& )),
             SLOT( slotEntryAdded( const KonqHistoryEntry& )));
    connect( mgr, SIGNAL( entryRemoved( const KonqHistoryEntry& )),
             SLOT( slotEntryRemoved( const KonqHistoryEntry& )));
    connect( mgr, SIGNAL( cleared() ), SLOT( slotHistoryCleared() ));

    s_mostEntries = new KonqHistoryList; // TODO KStaticDeleter
    const KonqHistoryList mgrEntries = mgr->entries();
    KonqHistoryList::const_iterator it = mgrEntries.begin();
    const KonqHistoryList::const_iterator end = mgrEntries.end();
    for ( uint i = 0; it != end && i < s_maxEntries; ++i, ++it ) {
	s_mostEntries->append( *it );
    }
    qSort( s_mostEntries->begin(), s_mostEntries->end(), numberOfVisitOrder );

    while ( it != end ) {
	const KonqHistoryEntry& leastOften = s_mostEntries->first();
	const KonqHistoryEntry& entry = *it;
	if ( leastOften.numberOfTimesVisited < entry.numberOfTimesVisited ) {
	    s_mostEntries->removeFirst();
	    inSort( entry );
	}

	++it;
    }
}

void KonqMostOftenURLSAction::slotEntryAdded( const KonqHistoryEntry& entry )
{
    // if it's already present, remove it, and inSort it
    s_mostEntries->removeEntry( entry.url );

    if ( s_mostEntries->count() >= s_maxEntries ) {
	const KonqHistoryEntry& leastOften = s_mostEntries->first();
	if ( leastOften.numberOfTimesVisited < entry.numberOfTimesVisited ) {
	    s_mostEntries->removeFirst();
	    inSort( entry );
	}
    }

    else
	inSort( entry );
    setEnabled( !s_mostEntries->isEmpty() );
}

void KonqMostOftenURLSAction::slotEntryRemoved( const KonqHistoryEntry& entry )
{
    s_mostEntries->removeEntry( entry.url );
    setEnabled( !s_mostEntries->isEmpty() );
}

void KonqMostOftenURLSAction::slotHistoryCleared()
{
    s_mostEntries->clear();
    setEnabled( false );
}

void KonqMostOftenURLSAction::slotFillMenu()
{
    if ( !s_mostEntries ) // first time
	parseHistory();

    menu()->clear();
    m_popupList.clear();

    for ( int id = s_mostEntries->count() - 1; id >= 0; --id ) {
        const KonqHistoryEntry entry = s_mostEntries->at( id );
	// we take either title, typedUrl or URL (in this order)
	QString text = entry.title.isEmpty() ? (entry.typedUrl.isEmpty() ?
						 entry.url.prettyUrl() :
						 entry.typedUrl) :
		       entry.title;

	menu()->insertItem(
		    QIcon(KonqPixmapProvider::self()->pixmapFor( entry.url.url() )),
		    text, id );
        // Keep a copy of the URLs being shown in the menu
        // This prevents crashes when another process tells us to remove an entry.
        m_popupList.prepend( entry.url );
    }
    setEnabled( !s_mostEntries->isEmpty() );
    Q_ASSERT( (int)s_mostEntries->count() == m_popupList.count() );
}

#if 0
void KonqMostOftenURLSAction::slotClearMenu()
{
    // Warning this is called _before_ slotActivated, when activating a menu item.
    // So e.g. don't clear m_popupList here.
}
#endif

void KonqMostOftenURLSAction::slotActivated( int id )
{
    Q_ASSERT( !m_popupList.isEmpty() ); // can not happen
    Q_ASSERT( id < (int)m_popupList.count() );

    KUrl url = m_popupList[ id ];
    if ( url.isValid() )
	emit activated( url );
    else
	kWarning() << "Invalid url: " << url.prettyUrl() << endl;
    m_popupList.clear();
}

#include "konq_actions.moc"
