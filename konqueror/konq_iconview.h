/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/     

#ifndef __konq_iconview_h__
#define __konq_iconview_h__

#include "kiconcontainer.h"
#include "kfileicon.h"
#include "konq_baseview.h"

#include <qtimer.h>
#include <qstrlist.h>

#include <list>

class KonqKfmIconView;
class KonqKfmIconViewItem;
class KMimeType;

class KonqKfmIconViewItem : public KIconContainerItem,
                            public KFileIcon
{
public:
  KonqKfmIconViewItem( KonqKfmIconView *_parent, UDSEntry& _entry, KURL& _url);
  virtual ~KonqKfmIconViewItem() { }

protected:
  void init();
  virtual void paint( QPainter* _painter, bool _drag );
  virtual void refresh();
  
  KonqKfmIconView* m_pIconView;
};

class KonqKfmIconView : public KIconContainer,
                     public KonqBaseView,
		     virtual public Konqueror::KfmIconView_skel
{
  Q_OBJECT
public:
  KonqKfmIconView( QWidget *_parent = 0L );
  virtual ~KonqKfmIconView();

  virtual bool mappingOpenURL( Konqueror::EventOpenURL eventURL );
  virtual bool mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu );
  virtual bool mappingFillMenuEdit( Konqueror::View::EventFillMenu editMenu );

  virtual void stop();
  virtual char *viewName() { return CORBA::string_dup("KonquerorKfmIconView"); }
  
  virtual char *url();
      
  virtual void openURL( const char* _url );
  
  virtual void selectedItems( list<KonqKfmIconViewItem*>& _list );

  virtual void updateDirectory();

public slots:
  virtual void slotCloseURL( int _id );
  virtual void slotListEntry( int _id, const UDSEntry& _entry );
  virtual void slotError( int _id, int _errid, const char *_errortext );
  
  virtual void slotBufferTimeout();

  // IDL
  virtual void slotLargeIcons();
  virtual void slotSmallIcons();
  virtual void slotShowDot();
  virtual void slotSelect();
  virtual void slotSelectAll();

protected slots:
  virtual void slotMousePressed( KIconContainerItem* _item, const QPoint& _global, int _button );
  virtual void slotDoubleClicked( KIconContainerItem* _item, const QPoint& _global, int _button );
  virtual void slotReturnPressed( KIconContainerItem* _item, const QPoint& _global );
  virtual void slotDrop( QDropEvent*, KIconContainerItem*, QStrList& _formats );
  
  virtual void slotUpdateError( int _id, int _errid, const char *_errortext );
  virtual void slotUpdateFinished( int _id );
  virtual void slotUpdateListEntry( int _id, const UDSEntry& _entry );

  virtual void slotOnItem( KIconContainerItem* );

protected:  
  virtual void initConfig();

  virtual void focusInEvent( QFocusEvent* _event );

  KURL m_url;
  bool m_bIsLocalURL;
  
  int m_jobId;

  bool m_bComplete;
  QString m_sWorkingURL;
  //KURL m_workingURL;

  list<UDSEntry> m_buffer;
  QTimer m_bufferTimer;

  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit;
};

#endif
