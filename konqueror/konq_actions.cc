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
#include <qcombobox.h>

#include <ktoolbar.h>

extern int get_toolbutton_id();

KonqComboAction::KonqComboAction( const QString& text, int accel, const QObject *receiver, const char *member,
			          QObject* parent, const char* name )
    : QAction( text, accel, parent, name )
{
  m_receiver = receiver;
  m_member = member;
}

int KonqComboAction::plug( QWidget *w )
{
  //  if ( !w->inherits( "KToolBar" ) );
  //    return -1;

  KToolBar *toolBar = (KToolBar *)w;

  QLabel *label = new QLabel( plainText(), w );
  label->adjustSize();
  toolBar->insertWidget( get_toolbutton_id(), label->width(), label );

  int id = get_toolbutton_id();

  toolBar->insertCombo( m_items, id, true, SIGNAL( activated( const QString & ) ),m_receiver, m_member );

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
  int id = menuId( idx ) + 1;

  QWidget *l = toolBar->getWidget( id );

  assert( l->inherits( "QLabel" ) );

  toolBar->removeItem( id );

  toolBar->removeItem( menuId( idx ) );

  removeContainer( idx );
}

KonqHistoryAction::KonqHistoryAction( const QString& text, int accel, QObject* parent, const char* name )
  : KAction( text, accel, parent, name )
{
  m_popup = 0;
}

KonqHistoryAction::KonqHistoryAction( const QString& text, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name )
  : KAction( text, accel, receiver, slot, parent, name )
{
  m_popup = 0;
}

KonqHistoryAction::KonqHistoryAction( const QString& text, const QIconSet& pix, int accel, QObject* parent, const char* name )
  : KAction( text, pix, accel, parent, name )
{
  m_popup = 0;
}

KonqHistoryAction::KonqHistoryAction( const QString& text, const QIconSet& pix,int accel, QObject* receiver, const char* slot, QObject* parent, const char* name )
  : KAction( text, pix, accel, receiver, slot, parent, name )
{
  m_popup = 0;
}

KonqHistoryAction::KonqHistoryAction( QObject* parent, const char* name )
  : KAction( parent, name )
{
  m_popup = 0;
}

KonqHistoryAction::~KonqHistoryAction()
{
  if ( m_popup )
    delete m_popup;
}

int KonqHistoryAction::plug( QWidget *widget )
{

  if ( widget->inherits( "KToolBar" ) )
  {
    KToolBar *bar = (KToolBar *)widget;

    int id_ = get_toolbutton_id();
    bar->insertButton( iconSet().pixmap(), id_, SIGNAL( clicked() ), this, SLOT( slotActivated() ),
		       isEnabled(), plainText() );

    addContainer( bar, id_ );

    connect( bar, SIGNAL( destroyed() ), this, SLOT( slotDestroyed() ) );

    bar->setDelayedPopup( id_, popupMenu() );

    return containerCount() - 1;
  }

  return KAction::plug( widget );
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



#include "konq_actions.moc"
