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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_actions.h"

#include <assert.h>

#include <qlabel.h>
#include <qapplication.h>
#include <qdragobject.h>
#include <qpopupmenu.h>
#include <qwhatsthis.h>

#include <kurldrag.h>
#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kcombobox.h>
#include <kanimwidget.h>
#include <kdebug.h>
#include <kstringhandler.h>

#include <konq_pixmapprovider.h>
#include "konq_view.h" // HistoryEntry

template class QList<KonqHistoryEntry>;


KonqComboAction::KonqComboAction( const QString& text, int accel, const QObject *receiver, const char *member,
                                  QObject* parent, const char* name )
    : KAction( text, accel, parent, name )
{
  m_receiver = receiver;
  m_member = member;
}

KonqComboAction::~KonqComboAction()
{
}

int KonqComboAction::plug( QWidget *w, int index )
{
  //  if ( !w->inherits( "KToolBar" ) );
  //    return -1;

  KToolBar *toolBar = (KToolBar *)w;

  int id = KAction::getToolButtonID();
  //kdDebug() << "KonqComboAction::plug id=" << id << endl;

  KonqCombo *comboBox = new KonqCombo( toolBar, "history combo" );
  toolBar->insertWidget( id, 70, comboBox, index );
  connect( comboBox, SIGNAL( activated( const QString& )), m_receiver, m_member );

  addContainer( toolBar, id );

  connect( toolBar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

  toolBar->setItemAutoSized( id, true );

  m_combo = comboBox;

  emit plugged();

  QWhatsThis::add( comboBox, whatsThis() );

  return containerCount() - 1;
}

void KonqComboAction::unplug( QWidget *w )
{
//  if ( !w->inherits( "KToolBar" ) )
//    return;

  KToolBar *toolBar = (KToolBar *)w;

  int idx = findContainer( w );
  //kdDebug() << "KonqComboAction::unplug idx=" << idx << " menuId=" << menuId(idx) << endl;

  toolBar->removeItem( menuId( idx ) );

  removeContainer( idx );
  m_combo = 0L;
}

/////////////////

//static - used by KonqHistoryAction and KonqBidiHistoryAction
void KonqBidiHistoryAction::fillHistoryPopup( const QList<HistoryEntry> &history,
                                          QPopupMenu * popup,
                                          bool onlyBack,
                                          bool onlyForward,
                                          bool checkCurrentItem,
                                          uint startPos )
{
  assert ( popup ); // kill me if this 0... :/

  //kdDebug(1202) << "fillHistoryPopup position: " << history.at() << endl;
  HistoryEntry * current = history.current();
  QListIterator<HistoryEntry> it( history );
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
      text = KStringHandler::csqueeze(text, 50); //CT: squeeze
      text.replace( QRegExp( "&" ), "&&" );
      if ( checkCurrentItem && it.current() == current )
      {
          int id = popup->insertItem( text ); // no pixmap if checked
          popup->setItemChecked( id, true );
      } else
          popup->insertItem( KonqPixmapProvider::self()->pixmapFor(
					    it.current()->url.url() ), text );
      if ( ++i > 10 )
          break;
      if ( !onlyForward ) --it; else ++it;
  }
  //kdDebug(1202) << "After fillHistoryPopup position: " << history.at() << endl;
}

///////////////////////////////

KonqBidiHistoryAction::KonqBidiHistoryAction ( const QString & text, QObject* parent, const char* name )
  : KAction( text, 0, parent, name )
{
  m_firstIndex = 0;
  m_goMenu = 0L;
}

int KonqBidiHistoryAction::plug( QWidget *widget, int index )
{
  // Go menu
  if ( widget->inherits("QPopupMenu") )
  {
    m_goMenu = (QPopupMenu*)widget;
    // Forward signal (to main view)
    connect( m_goMenu, SIGNAL( aboutToShow() ),
             this, SIGNAL( menuAboutToShow() ) );
    connect( m_goMenu, SIGNAL( activated( int ) ),
             this, SLOT( slotActivated( int ) ) );
    //kdDebug(1202) << "m_goMenu->count()=" << m_goMenu->count() << endl;
    // Store how many items the menu already contains.
    // This means, the KonqBidiHistoryAction has to be plugged LAST in a menu !
    m_firstIndex = m_goMenu->count();
    return m_goMenu->count(); // hmmm, what should this be ?
  }
  return KAction::plug( widget, index );
}

void KonqBidiHistoryAction::fillGoMenu( const QList<HistoryEntry> & history )
{
    if (history.isEmpty())
        return; // nothing to do

    //kdDebug(1202) << "fillGoMenu position: " << history.at() << endl;
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
    ASSERT( m_startPos >= 0 && (uint)m_startPos < history.count() );
    if ( m_startPos < 0 || (uint)m_startPos >= history.count() )
    {
        kdWarning() << "m_startPos=" << m_startPos << " history.count()=" << history.count() << endl;
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
      kdDebug(1202) << "Item clicked has index " << index << endl;
      // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
      int steps = ( m_startPos+1 ) - index - m_currentPos; // make a drawing to understand this :-)
      kdDebug(1202) << "Emit activated with steps = " << steps << endl;
      emit activated( steps );
  }
}

///////////////////////////////

KonqLogoAction::KonqLogoAction( const QString& text, int accel, QObject* parent, const char* name )
  : KAction( text, accel, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const QString& text, int accel,
                               QObject* receiver, const char* slot, QObject* parent, const char* name )
  : KAction( text, accel, receiver, slot, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const QString& text, const QIconSet& pix, int accel, QObject* parent, const char* name )
  : KAction( text, pix, accel, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const QString& text, const QIconSet& pix,int accel, QObject* receiver, const char* slot, QObject* parent, const char* name )
  : KAction( text, pix, accel, receiver, slot, parent, name )
{
}

KonqLogoAction::KonqLogoAction( const QStringList& icons, QObject* receiver,
                                const char* slot, QObject* parent,
                                const char* name )
    : KAction( 0L, 0L, receiver, slot, parent, name ) // text missing !
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
      KAnimWidget *anim = ((KToolBar *)w)->animatedWidget( menuId( i ) );
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
      KAnimWidget *anim = ((KToolBar *)w)->animatedWidget( menuId( i ) );
      anim->stop();
    }
  }
}

int KonqLogoAction::plug( QWidget *widget, int index )
{
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


class KonqDraggableLabel : public QLabel
{
public:
    KonqDraggableLabel( KonqMainWindow * mw, const QString & text, QWidget * parent = 0, const char * name = 0 )
        : QLabel( text, parent, name ), m_mw(mw)
    { validDrag = false; }
protected:
    void mousePressEvent( QMouseEvent * ev )
    {
        validDrag = true;
        startDragPos = ev->pos();
    }
    void mouseMoveEvent( QMouseEvent * ev )
    {
        if ((startDragPos - ev->pos()).manhattanLength() > QApplication::startDragDistance())
        {
            validDrag = false;
            if ( m_mw->currentView() )
            {
                KURL::List lst;
                lst.append( m_mw->currentView()->url() );
                QDragObject * drag = KURLDrag::newDrag( lst, m_mw );
                drag->setPixmap( KMimeType::pixmapForURL( lst.first(), 0, KIcon::Small ) );
                (void) drag->drag();
            }
        }
    }
    void mouseReleaseEvent( QMouseEvent * )
    {
        validDrag = false;
    }
private:
    QPoint startDragPos;
    bool validDrag;
    KonqMainWindow * m_mw;
};

KonqLabelAction::KonqLabelAction( const QString &text, QObject *parent, const char *name )
    : KAction( text, 0, parent, name ), m_label( 0L )
{
}

int KonqLabelAction::plug( QWidget *widget, int index )
{
  //do not call the previous implementation here

  if ( widget->inherits( "KToolBar" ) )
  {
    KToolBar *tb = (KToolBar *)widget;

    int id = KAction::getToolButtonID();

    m_label = new KonqDraggableLabel( static_cast<KonqMainWindow *>(tb->mainWindow()), text(), widget );
    m_label->setAlignment( Qt::AlignLeft | Qt::AlignVCenter | Qt::ShowPrefix );
    m_label->adjustSize();
    tb->insertWidget( id, m_label->width(), m_label, index );

    addContainer( tb, id );

    connect( tb, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

    return containerCount() - 1;
  }

  return -1;
}

void KonqLabelAction::unplug( QWidget *widget )
{
  if ( widget->inherits( "KToolBar" ) )
  {
    KToolBar *bar = (KToolBar *)widget;

    int idx = findContainer( bar );

    if ( idx != -1 )
    {
      bar->removeItem( menuId( idx ) );
      removeContainer( idx );
    }

    m_label = 0;
    return;
  }
}

KonqViewModeAction::KonqViewModeAction( const QString &text, const QString &icon,
                                        QObject *parent, const char *name )
    : KRadioAction( text, icon, 0, parent, name )
{
    m_menu = new QPopupMenu;

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

    if ( widget->inherits( "KToolBar" ) )
    {
        KToolBar *toolBar = static_cast<KToolBar *>( widget );

        KToolBarButton *button = toolBar->getButton( itemId( res ) );

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
bool KonqMostOftenURLSAction::s_bLocked = false;

KonqMostOftenURLSAction::KonqMostOftenURLSAction( const QString& text,
						  QObject *parent,
						  const char *name )
    : KActionMenu( text, "goto", parent, name )
{
    setDelayed( false );

    connect( popupMenu(), SIGNAL( aboutToShow() ), SLOT( slotFillMenu() ));
    connect( popupMenu(), SIGNAL( aboutToHide() ), SLOT( slotClearMenu() ));
    connect( popupMenu(), SIGNAL( activated( int ) ),
	     SLOT( slotActivated(int) ));
}

KonqMostOftenURLSAction::~KonqMostOftenURLSAction()
{
}

void KonqMostOftenURLSAction::parseHistory()
{
    bool didInit = false;
    if ( !s_mostEntries ) {
	KConfig *kc = KGlobal::config();
	KConfigGroupSaver cs( kc, "Settings" );
	s_maxEntries = kc->readNumEntry( "Number of most visited URLs", 10 );

	s_mostEntries = new MostOftenList; // exit() will clean this up for now
	didInit = true;
    }

    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    KonqHistoryIterator it( mgr->entries() );

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

    if ( didInit ) {
	connect( mgr, SIGNAL( entryAdded( const KonqHistoryEntry * )),
		 SLOT( slotEntryAdded( const KonqHistoryEntry * )));
	connect( mgr, SIGNAL( entryRemoved( const KonqHistoryEntry * )),
		 SLOT( slotEntryRemoved( const KonqHistoryEntry * )));
	connect( mgr, SIGNAL( cleared() ), SLOT( slotHistoryCleared() ));
    }
}

void KonqMostOftenURLSAction::slotEntryAdded( const KonqHistoryEntry *entry )
{
    // don't disturb, a popup is visible right now!
    if ( s_bLocked )
	return;

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
}

void KonqMostOftenURLSAction::slotEntryRemoved( const KonqHistoryEntry *entry )
{
    // don't disturb, a popup is visible right now!
    if ( s_bLocked )
	return;

    s_mostEntries->removeRef( entry );
}

void KonqMostOftenURLSAction::slotHistoryCleared()
{
    // don't disturb, a popup is visible right now! Pretty much impossible tho.
    if ( s_bLocked )
	return;

    s_mostEntries->clear();
}

void KonqMostOftenURLSAction::slotFillMenu()
{
    if ( !s_mostEntries )
	parseHistory();

    s_bLocked = true;
    popupMenu()->clear();

    int id = s_mostEntries->count() -1;
    KonqHistoryEntry *entry = s_mostEntries->at( id );
    while ( entry ) {
	// we take either title, typedURL or URL (in this order)
	QString text = entry->title.isEmpty() ? (entry->typedURL.isEmpty() ?
						 entry->url.prettyURL() :
						 entry->typedURL) :
		       entry->title;

	popupMenu()->insertItem(
		    KonqPixmapProvider::self()->pixmapFor( entry->url.url() ),
		    text, id );

	entry = (id > 0) ? s_mostEntries->at( --id ) : 0L;
    }
}

void KonqMostOftenURLSAction::slotClearMenu()
{
    s_bLocked = false;
}

void KonqMostOftenURLSAction::slotActivated( int id )
{
    ASSERT( s_mostEntries ); // can basically not happen

    const KonqHistoryEntry *entry = s_mostEntries->at( id );
    KURL url = entry ? entry->url : KURL();
    if ( url.isValid() )
	emit activated( url );
    else
	kdWarning() << "Invalid url: " << url.prettyURL() << endl;
}

// sort by numberOfTimesVisited (least often goes first)
int MostOftenList::compareItems( QCollection::Item item1,
				 QCollection::Item item2)
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

KonqGoURLAction::KonqGoURLAction( const QString &text, const QString &pix,
                                  int accel, const QObject *receiver,
                                  const char *slot, QObject *parent,
                                  const char *name )
    : KAction( text, pix, accel, receiver, slot, parent, name )
{
}

int KonqGoURLAction::plug( QWidget *widget, int index )
{
    int container = KAction::plug( widget, index );

    if ( widget->inherits( "KToolBar" ) && container != -1 )
    {
        int toolButtonId = itemId( container );
        static_cast<KToolBar *>( widget )->alignItemRight( toolButtonId, true );
    }

    return container;
}

#include "konq_actions.moc"
