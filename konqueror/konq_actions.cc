/* This file is part of the KDE project
   Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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

#include "konq_actions.h"

#include <assert.h>

#include <qlabel.h>
#include <qpopupmenu.h>
#include <qcombobox.h>

#include <ktoolbar.h>
#include <kanimwidget.h>
#include <kdebug.h>
#include <konq_childview.h> // HistoryEntry

KonqComboAction::KonqComboAction( const QString& text, int accel, const QObject *receiver, const char *member,
			          QObject* parent, const char* name )
    : KAction( text, accel, parent, name )
{
  m_receiver = receiver;
  m_member = member;
}

int KonqComboAction::plug( QWidget *w, int index )
{
  //  if ( !w->inherits( "KToolBar" ) );
  //    return -1;

  KToolBar *toolBar = (KToolBar *)w;

  int id = KAction::getToolButtonID();

  toolBar->insertCombo( m_items, id, true, SIGNAL( activated( const QString & ) ),m_receiver, m_member, true, QString::null, 70, index );

  QComboBox *comboBox = toolBar->getCombo( id );

  addContainer( toolBar, id );

  connect( toolBar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

  toolBar->setItemAutoSized( id, true );
  comboBox->setAutoCompletion( true );
  comboBox->setMaxCount( 10 );
  comboBox->setInsertionPolicy( QComboBox::AfterCurrent );

  m_combo = comboBox;

  emit plugged();

  return containerCount() - 1;
}

void KonqComboAction::unplug( QWidget *w )
{
//  if ( !w->inherits( "KToolBar" ) )
//    return;

  KToolBar *toolBar = (KToolBar *)w;

  int idx = findContainer( w );

  toolBar->removeItem( menuId( idx ) );

  removeContainer( idx );
}

/////////////////

KonqHistoryAction::KonqHistoryAction( const QString& text, const QString& icon, int accel, QObject* parent, const char* name )
  : KAction( text, icon, accel, parent, name )
{
  m_popup = 0;
}

KonqHistoryAction::~KonqHistoryAction()
{
  if ( m_popup )
    delete m_popup;
}

int KonqHistoryAction::plug( QWidget *widget, int index )
{

  if ( widget->inherits( "KToolBar" ) )
  {
    KToolBar *bar = (KToolBar *)widget;

    int id_ = KAction::getToolButtonID();
    bar->insertButton( iconName(), id_, SIGNAL( clicked() ), this,
                       SLOT( slotActivated() ), isEnabled(), plainText(),
                       index );

    addContainer( bar, id_ );

    connect( bar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

    bar->setDelayedPopup( id_, popupMenu(), true );

    return containerCount() - 1;
  }

  return KAction::plug( widget, index );
}

void KonqHistoryAction::unplug( QWidget *widget )
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

    return;
  }

  KAction::unplug( widget );
}

//static - used by KonqHistoryAction and KonqBidiHistoryAction
void KonqHistoryAction::fillHistoryPopup( const QList<HistoryEntry> &history,
                                          QPopupMenu * popup,
                                          bool onlyBack,
                                          bool onlyForward,
                                          bool checkCurrentItem,
                                          uint startPos )
{
  assert ( popup ); // kill me if this 0... :/

  kdDebug(1202) << "fillHistoryPopup position: " << history.at() << endl;
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
      QString text = it.current()->locationBarURL; // perhaps the caption would look even better ?
      text = KBookmark::stringSqueeze(text, 40); //CT squeeze, but not as much as in bookmarks (now we display URLs)
      if ( checkCurrentItem && it.current() == current )
      {
          int id = popup->insertItem( text ); // no pixmap if checked
          popup->setItemChecked( id, true );
      } else
          popup->insertItem( KMimeType::mimeType( it.current()->strServiceType )->
		pixmap( KIcon::Small ), text );
      if ( ++i > 10 )
          break;
      if ( !onlyForward ) --it; else ++it;
  }
  kdDebug(1202) << "After fillHistoryPopup position: " << history.at() << endl;
}

void KonqHistoryAction::setEnabled( bool b )
{
  // Is this still necessary ? Looks very standard...

  int len = containerCount();

  for ( int i = 0; i < len; i++ )
  {
    QWidget *w = container( i );

    if ( w->inherits( "KToolBar" ) )
      ((KToolBar *)w)->setItemEnabled( menuId( i ), b );

  }

  KAction::setEnabled( b );
}

void KonqHistoryAction::setIconSet( const QIconSet& iconSet )
{
  // Is this still necessary ? Looks very standard...

  int len = containerCount();

  for ( int i = 0; i < len; i++ )
  {
    QWidget *w = container( i );

    if ( w->inherits( "KToolBar" ) )
      ((KToolBar *)w)->setButtonPixmap( menuId( i ), iconSet.pixmap() );

  }

  KAction::setIconSet( iconSet );
}

QPopupMenu *KonqHistoryAction::popupMenu()
{
  if ( m_popup )
    return m_popup;

  m_popup = new QPopupMenu();
  return m_popup;
}

///////////////////////////////

KonqBidiHistoryAction::KonqBidiHistoryAction ( QObject* parent, const char* name )
  : KAction( QString::null, 0, parent, name )
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
    return m_goMenu->count() /* hmmm, what should this be ? */;
  }
  return KAction::plug( widget, index );
}

void KonqBidiHistoryAction::fillGoMenu( const QList<HistoryEntry> & history )
{
    kdDebug(1202) << "fillGoMenu position: " << history.at() << endl;
    // Tricky. The first time, the menu doesn't contain history
    // (but contains the other actions) -> store count at that point
    if ( m_firstIndex == 0 )
    {
        m_firstIndex = m_goMenu->count();
    }
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
    assert( m_startPos >= 0 && (uint)m_startPos < history.count() );
    m_currentPos = history.at(); // for slotActivated
    KonqHistoryAction::fillHistoryPopup( history, m_goMenu, false, false, true, m_startPos );
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
  : KAction( 0L, 0L, receiver, slot, parent, name )
{
  iconList = icons;
}

KonqLogoAction::KonqLogoAction( QObject* parent, const char* name )
  : KAction( parent, name )
{
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
  if ( widget->inherits( "KTMainWindow" ) )
  {
    ((KTMainWindow*)widget)->setIndicatorWidget(m_logoLabel);

    addContainer( widget, -1 );

    return containerCount() - 1;
  }
*/
  if ( widget->inherits( "KToolBar" ) )
  {
    KToolBar *bar = (KToolBar *)widget;

    int id_ = getToolButtonID();

    bar->insertAnimatedWidget( id_, this, SIGNAL(activated()), iconList, index );
    bar->alignItemRight( id_ );

    addContainer( bar, id_ );

    connect( bar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

    return containerCount() - 1;
  }

  int containerId = KAction::plug( widget, index );

  return containerId;
}

KonqLabelAction::KonqLabelAction( const QString &text, QObject *parent, const char *name )
: KAction( text, 0, parent, name )
{
}

int KonqLabelAction::plug( QWidget *widget, int index )
{
  //do not call the previous implementation here

  if ( widget->inherits( "KToolBar" ) )
  {
    KToolBar *tb = (KToolBar *)widget;

    int id = KAction::getToolButtonID();

    QLabel *label = new QLabel( plainText(), widget );
    label->adjustSize();
    tb->insertWidget( id, label->width(), label, index );

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

    return;
  }
}

#include "konq_actions.moc"
