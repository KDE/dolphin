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

#include <ktoolbarbutton.h>
#include <kanimwidget.h>
#include <kdebug.h>
#include <kstringhandler.h>

#include <konq_pixmapprovider.h>
#include <kiconloader.h>
#include <kmenu.h>
#include <kapplication.h>

#include "konq_view.h"
#include "konq_settingsxt.h"
//Added by qt3to4:
#include <Q3PtrList>
#include <kauthorized.h>

template class Q3PtrList<KonqHistoryEntry>;

/////////////////

//static - used by KonqHistoryAction and KonqBidiHistoryAction
void KonqBidiHistoryAction::fillHistoryPopup( const Q3PtrList<HistoryEntry> &history,
                                          QMenu * popup,
                                          bool onlyBack,
                                          bool onlyForward,
                                          bool checkCurrentItem,
                                          uint startPos )
{
  assert ( popup ); // kill me if this 0... :/

  //kDebug(1202) << "fillHistoryPopup position: " << history.at() << endl;
  HistoryEntry * current = history.current();
  Q3PtrListIterator<HistoryEntry> it( history );
  if (onlyBack || onlyForward)
  {
      it += history.at(); // Jump to current item
      if ( !onlyForward ) --it; else ++it; // And move off it
  } else if ( startPos )
      it += startPos; // Jump to specified start pos

  uint i = 0;
  while ( it.current() )
  {
      QString text = it.current()->title;
      text = KStringHandler::cEmSqueeze(text, popup->fontMetrics(), 30); //CT: squeeze
      text.replace( "&", "&&" );
      if ( checkCurrentItem && it.current() == current )
      {
          int id = popup->insertItem( text ); // no pixmap if checked
          popup->setItemChecked( id, true );
      } else
          popup->insertItem( QIcon( KonqPixmapProvider::self()->pixmapFor(
					    it.current()->url.url() ) ), text );
      if ( ++i > 10 )
          break;
      if ( !onlyForward ) --it; else ++it;
  }
  //kDebug(1202) << "After fillHistoryPopup position: " << history.at() << endl;
}

///////////////////////////////

KonqBidiHistoryAction::KonqBidiHistoryAction ( const QString & text, KActionCollection* parent, const char* name )
  : KAction( text, KShortcut(), 0L, "", parent, name )
{
  setShortcutConfigurable(false);
  m_firstIndex = 0;
  m_goMenu = 0L;
}

int KonqBidiHistoryAction::plug( QWidget *widget, int index )
{
  if (!KAuthorized::authorizeKAction(name()))
    return -1;

  // Go menu
  if ( widget->inherits("QPopupMenu") )
  {
    m_goMenu = (QMenu*)widget;
    // Forward signal (to main view)
    connect( m_goMenu, SIGNAL( aboutToShow() ),
             this, SIGNAL( menuAboutToShow() ) );
    connect( m_goMenu, SIGNAL( activated( int ) ),
             this, SLOT( slotActivated( int ) ) );
    //kDebug(1202) << "m_goMenu->count()=" << m_goMenu->count() << endl;
    // Store how many items the menu already contains.
    // This means, the KonqBidiHistoryAction has to be plugged LAST in a menu !
    m_firstIndex = m_goMenu->count();
    return m_goMenu->count(); // hmmm, what should this be ?
  }
  return KAction::plug( widget, index );
}

void KonqBidiHistoryAction::fillGoMenu( const Q3PtrList<HistoryEntry> & history )
{
    if (history.isEmpty())
        return; // nothing to do

    //kDebug(1202) << "fillGoMenu position: " << history.at() << endl;
    if ( m_firstIndex == 0 ) // should never happen since done in plug
        m_firstIndex = m_goMenu->count();
    else
    { // Clean up old history (from the end, to avoid shifts)
        for ( uint i = m_goMenu->count()-1 ; i >= m_firstIndex; i-- )
            m_goMenu->removeItemAt( i );
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
        m_startPos = history.at() + 4;

        // Forward not big enough ?
        if ( history.at() > (int)history.count() - 4 )
          m_startPos = history.count() - 1;
    }
    Q_ASSERT( m_startPos >= 0 && (uint)m_startPos < history.count() );
    if ( m_startPos < 0 || (uint)m_startPos >= history.count() )
    {
        kWarning() << "m_startPos=" << m_startPos << " history.count()=" << history.count() << endl;
        return;
    }
    m_currentPos = history.at(); // for slotActivated
    KonqBidiHistoryAction::fillHistoryPopup( history, m_goMenu, false, false, true, m_startPos );
}

void KonqBidiHistoryAction::slotActivated( int id )
{
  // 1 for first item in the list, etc.
  int index = m_goMenu->indexOf(id) - m_firstIndex + 1;
  if ( index > 0 )
  {
      kDebug(1202) << "Item clicked has index " << index << endl;
      // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
      int steps = ( m_startPos+1 ) - index - m_currentPos; // make a drawing to understand this :-)
      kDebug(1202) << "Emit activated with steps = " << steps << endl;
      emit activated( steps );
  }
}

///////////////////////////////

KonqLogoAction::KonqLogoAction( const QString& text, const KShortcut& accel, KActionCollection* parent, const char* name )
  : KAction( text, accel, 0L, "", parent, name )
{
}

KonqLogoAction::KonqLogoAction( const QString& text, const KShortcut& accel,
                               QObject* receiver, const char* slot, KActionCollection* parent, const char* name )
  : KAction( text, accel, receiver, slot, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const QString& text, const QIcon& pix, const KShortcut&  accel, KActionCollection* parent, const char* name )
  : KAction( text, pix, accel, 0L, "", parent, name )
{
}

KonqLogoAction::KonqLogoAction( const QString& text, const QIcon& pix, const KShortcut& accel, QObject* receiver, const char* slot, KActionCollection* parent, const char* name )
  : KAction( text, pix, accel, receiver, slot, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const QStringList& icons, QObject* receiver,
                                const char* slot, KActionCollection* parent,
                                const char* name )
    : KAction( 0L, 0, receiver, slot, parent, name ) // text missing !
{
  iconList = icons;
}

void KonqLogoAction::start()
{
  int len = containerCount();
  for ( int i = 0; i < len; i++ )
  {
    QWidget *w = container( i );

    if ( w->inherits( "KToolBar" ) )
    {
      KAnimWidget *anim = ((KToolBar *)w)->animatedWidget( itemId( i ) );
      anim->start();
    }
  }
}

void KonqLogoAction::stop()
{
  int len = containerCount();
  for ( int i = 0; i < len; i++ )
  {
    QWidget *w = container( i );

    if ( w->inherits( "KToolBar" ) )
    {
      KAnimWidget *anim = ((KToolBar *)w)->animatedWidget( itemId( i ) );
      anim->stop();
    }
  }
}

void KonqLogoAction::updateIcon(int id)
{
    QWidget *w = container( id );

    if ( w->inherits( "KToolBar" ) )
    {
      KAnimWidget *anim = ((KToolBar *)w)->animatedWidget( itemId( id ) );
      anim->setIcons(icon());
    }
}



int KonqLogoAction::plug( QWidget *widget, int index )
{
  if (!KAuthorized::authorizeKAction(name()))
    return -1;

/*
  if ( widget->inherits( "KMainWindow" ) )
  {
    ((KMainWindow*)widget)->setIndicatorWidget(m_logoLabel);

    addContainer( widget, -1 );

    return containerCount() - 1;
  }
*/
  if ( widget->inherits( "KToolBar" ) )
  {
    KToolBar *bar = (KToolBar *)widget;

    int id_ = getToolButtonID();

    bar->insertAnimatedWidget( id_, this, SIGNAL(activated()), QString("kde"), index );
    bar->alignItemRight( id_ );

    addContainer( bar, id_ );

    connect( bar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

    return containerCount() - 1;
  }

  int containerId = KAction::plug( widget, index );

  return containerId;
}

///////////

KonqViewModeAction::KonqViewModeAction( const QString& desktopEntryName,
                                        const QString &text, const QString &icon,
                                        KActionCollection *parent, const char *name )
    : KRadioAction( text, icon, 0, 0L, "", parent, name ),
      m_desktopEntryName( desktopEntryName )
{
    m_menu = new QMenu;

    connect( m_menu, SIGNAL( aboutToShow() ),
             this, SLOT( slotPopupAboutToShow() ) );
    connect( m_menu, SIGNAL( activated( int ) ),
             this, SLOT( slotPopupActivated() ) );
    connect( m_menu, SIGNAL( aboutToHide() ),
             this, SLOT( slotPopupAboutToHide() ) );
}

KonqViewModeAction::~KonqViewModeAction()
{
    delete m_menu;
}

int KonqViewModeAction::plug( QWidget *widget, int index )
{
    int res = KRadioAction::plug( widget, index );

    if ( widget->inherits( "KToolBar" ) && (res != -1) )
    {
        KToolBar *toolBar = static_cast<KToolBar *>( widget );

        KToolBarButton *button = toolBar->getButton( itemId( res ) );

        if ( m_menu->count() > 1 )
            button->setDelayedPopup( m_menu, false );
    }

    return res;
}

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
}


MostOftenList * KonqMostOftenURLSAction::s_mostEntries = 0L;
uint KonqMostOftenURLSAction::s_maxEntries = 0;

KonqMostOftenURLSAction::KonqMostOftenURLSAction( const QString& text,
						  KActionCollection *parent,
						  const char *name )
    : KActionMenu( text, "goto", parent, name )
{
    setDelayed( false );

    connect( popupMenu(), SIGNAL( aboutToShow() ), SLOT( slotFillMenu() ));
    //connect( popupMenu(), SIGNAL( aboutToHide() ), SLOT( slotClearMenu() ));
    connect( popupMenu(), SIGNAL( activated( int ) ),
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

void KonqMostOftenURLSAction::parseHistory() // only ever called once
{
    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    KonqHistoryIterator it( mgr->entries() );

    connect( mgr, SIGNAL( entryAdded( const KonqHistoryEntry * )),
             SLOT( slotEntryAdded( const KonqHistoryEntry * )));
    connect( mgr, SIGNAL( entryRemoved( const KonqHistoryEntry * )),
             SLOT( slotEntryRemoved( const KonqHistoryEntry * )));
    connect( mgr, SIGNAL( cleared() ), SLOT( slotHistoryCleared() ));

    s_mostEntries = new MostOftenList; // exit() will clean this up for now
    for ( uint i = 0; it.current() && i < s_maxEntries; i++ ) {
	s_mostEntries->append( it.current() );
	++it;
    }
    s_mostEntries->sort();

    while ( it.current() ) {
	KonqHistoryEntry *leastOften = s_mostEntries->first();
	KonqHistoryEntry *entry = it.current();
	if ( leastOften->numberOfTimesVisited < entry->numberOfTimesVisited ) {
	    s_mostEntries->removeFirst();
	    s_mostEntries->inSort( entry );
	}

	++it;
    }
}

void KonqMostOftenURLSAction::slotEntryAdded( const KonqHistoryEntry *entry )
{
    // if it's already present, remove it, and inSort it
    s_mostEntries->removeRef( entry );

    if ( s_mostEntries->count() >= s_maxEntries ) {
	KonqHistoryEntry *leastOften = s_mostEntries->first();
	if ( leastOften->numberOfTimesVisited < entry->numberOfTimesVisited ) {
	    s_mostEntries->removeFirst();
	    s_mostEntries->inSort( entry );
	}
    }

    else
	s_mostEntries->inSort( entry );
    setEnabled( !s_mostEntries->isEmpty() );
}

void KonqMostOftenURLSAction::slotEntryRemoved( const KonqHistoryEntry *entry )
{
    s_mostEntries->removeRef( entry );
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

    popupMenu()->clear();
    m_popupList.clear();

    int id = s_mostEntries->count() -1;
    KonqHistoryEntry *entry = s_mostEntries->at( id );
    while ( entry ) {
	// we take either title, typedURL or URL (in this order)
	QString text = entry->title.isEmpty() ? (entry->typedURL.isEmpty() ?
						 entry->url.prettyURL() :
						 entry->typedURL) :
		       entry->title;

	popupMenu()->insertItem(
		    QIcon(KonqPixmapProvider::self()->pixmapFor( entry->url.url() )),
		    text, id );
        // Keep a copy of the URLs being shown in the menu
        // This prevents crashes when another process tells us to remove an entry.
        m_popupList.prepend( entry->url );

	entry = (id > 0) ? s_mostEntries->at( --id ) : 0L;
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
	kWarning() << "Invalid url: " << url.prettyURL() << endl;
    m_popupList.clear();
}

// sort by numberOfTimesVisited (least often goes first)
int MostOftenList::compareItems( Q3PtrCollection::Item item1,
				 Q3PtrCollection::Item item2)
{
    KonqHistoryEntry *entry1 = static_cast<KonqHistoryEntry *>( item1 );
    KonqHistoryEntry *entry2 = static_cast<KonqHistoryEntry *>( item2 );

    if ( entry1->numberOfTimesVisited > entry2->numberOfTimesVisited )
	return 1;
    else if ( entry1->numberOfTimesVisited < entry2->numberOfTimesVisited )
	return -1;
    else
	return 0;
}

#include "konq_actions.moc"
