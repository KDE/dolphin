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
#include <konq_childview.h> // HistoryEntry

KonqComboAction::KonqComboAction( const QString& text, int accel, const QObject *receiver, const char *member,
			          QObject* parent, const char* name )
    : QAction( text, accel, parent, name )
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

KonqHistoryAction::KonqHistoryAction( const QString& text, int accel, QObject* parent, const char* name )
  : KAction( text, accel, parent, name )
{
  m_popup = 0;
  m_firstIndex = 0;
}

KonqHistoryAction::KonqHistoryAction( const QString& text, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name )
  : KAction( text, accel, receiver, slot, parent, name )
{
  m_popup = 0;
  m_firstIndex = 0;
}

KonqHistoryAction::KonqHistoryAction( const QString& text, const QIconSet& pix, int accel, QObject* parent, const char* name )
  : KAction( text, pix, accel, parent, name )
{
  m_popup = 0;
  m_firstIndex = 0;
}

KonqHistoryAction::KonqHistoryAction( const QString& text, const QIconSet& pix,int accel, QObject* receiver, const char* slot, QObject* parent, const char* name )
  : KAction( text, pix, accel, receiver, slot, parent, name )
{
  m_popup = 0;
  m_firstIndex = 0;
}

KonqHistoryAction::KonqHistoryAction( QObject* parent, const char* name )
  : KAction( parent, name )
{
  m_popup = 0;
  m_firstIndex = 0;
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
    bar->insertButton( iconSet().pixmap(), id_, SIGNAL( clicked() ), this, SLOT( slotActivated() ),
		       isEnabled(), plainText(), index );

    addContainer( bar, id_ );

    connect( bar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

    bar->setDelayedPopup( id_, popupMenu(), true );

    return containerCount() - 1;
  }
  if ( widget->inherits("QPopupMenu") )
  {
	m_goMenu = (QPopupMenu*)widget;
        // Forward signal (to main view)
        connect( m_goMenu, SIGNAL( aboutToShow() ),
                 this, SIGNAL( menuAboutToShow() ) );
        connect( m_goMenu, SIGNAL( activated( int ) ),
                 this, SLOT( slotActivated( int ) ) );
        // Do not return, we also want the default behaviour (get the item in the menu)
  }

  return KAction::plug( widget, index );
}

void KonqHistoryAction::fillGoMenu( const QList<HistoryEntry> &history )
{
    // Tricky. The first time, the menu doesn't contain history
    // (but contains the other actions) -> store count at that point
    if ( m_firstIndex == 0 )
    {
        m_firstIndex = m_goMenu->count();
    }
    else
    { // Clean up old history (from the end, to avoid shifts)
        for ( int i = m_goMenu->count()-1 ; i >= m_firstIndex; i-- )
            m_goMenu->removeItemAt( i );
    }
    // TODO perhaps smarter algorithm (rename existing items, create new ones only if not enough)
    fillHistoryPopup( history, m_goMenu );
}

void KonqHistoryAction::slotActivated( int id )
{
  emit activated( m_goMenu->indexOf(id) - m_firstIndex + 1 );
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

void KonqHistoryAction::fillHistoryPopup( const QList<HistoryEntry> &history,
                                          QPopupMenu * popup )
{
  if ( !popup )
    popup = popupMenu();

  QListIterator<HistoryEntry> it( history );
  uint i = 0;
  for (; it.current(); ++it )
  {
    popup->insertItem( KMimeType::mimeType( it.current()->strServiceType )->pixmap( KIconLoader::Small ),
                       it.current()->url.decodedURL() );
    if ( ++i > 10 )
      break;
  }

}

void KonqHistoryAction::setEnabled( bool b )
{
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

KonqLogoAction::KonqLogoAction( QObject* parent, const char* name )
  : KAction( parent, name )
{
}

int KonqLogoAction::plug( QWidget *widget, int index )
{
  int containerId = KAction::plug( widget, index );

  if ( widget->inherits( "KToolBar" ) )
  {
    ((KToolBar *)widget)->alignItemRight( menuId( containerId ) );
    ((KToolBar *)widget)->setItemNoStyle( menuId( containerId ) );
  }

  return containerId;
}

KonqLabelAction::KonqLabelAction( const QString &text, QObject *parent, const char *name )
: QAction( text, 0, parent, name )
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
