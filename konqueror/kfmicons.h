/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
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

#ifndef __kfm_icons_h__
#define __kfm_icons_h__

#include "kiconcontainer.h"

#include <qtimer.h>
#include <qstrlist.h>

#include <string>

#include <k2url.h>
#include <kio_interface.h>

class KfmIconView;
class KfmIconViewItem;
class KfmView;
class KMimeType;

class KfmIconViewItem : public KIconContainerItem
{
public:
  KfmIconViewItem( KfmIconView *_parent, UDSEntry& _entry, K2URL& _url, const char *_name );
  virtual ~KfmIconViewItem() { }

  virtual const char* url() { return m_strURL.c_str(); }

  // virtual void popupMenu( const QPoint& _global );
  virtual void returnPressed();

  virtual bool isMarked() { return m_bMarked; }
  virtual void mark() { m_bMarked = true; }
  virtual void unmark() { m_bMarked = false; }
  
  virtual UDSEntry udsEntry() { return m_entry; }
  
  virtual bool acceptsDrops( QStrList& _formats );

  virtual KMimeType* mimeType() { return m_pMimeType; }
protected:
  virtual void init( KfmIconView* _finder, UDSEntry& _entry, K2URL& _url, const char *_name );
  virtual void paint( QPainter* _painter, const QColorGroup _grp );

  virtual void refresh();
  
  UDSEntry m_entry;
  KMimeType* m_pMimeType;
  string m_strURL;
  KfmIconView* m_pParent;
  
  bool m_bIsLocalURL;
  KIconContainer::DisplayMode m_displayMode;

  bool m_bMarked;
};

class KfmIconView : public KIconContainer
{
  Q_OBJECT
public:
  KfmIconView( QWidget *_parent, KfmView *_parent );
  ~KfmIconView();
  
  virtual void openURL( const char* _url );
  
  virtual KfmView* view() { return m_pView; }
  virtual const char* url() { return m_strURL.c_str(); }
    
  virtual void selectedItems( list<KfmIconViewItem*>& _list );

  virtual void updateDirectory();
  
signals:
  void started( const char* _url );
  void completed();
  void canceled();

public slots:
  virtual void slotCloseURL( int _id );
  virtual void slotListEntry( int _id, UDSEntry& _entry );
  virtual void slotError( int _id, int _errid, const char *_errortext );
  
  virtual void slotBufferTimeout();

protected slots:
  virtual void slotMousePressed( KIconContainerItem* _item, const QPoint& _global, int _button );
  virtual void slotDoubleClicked( KIconContainerItem* _item, const QPoint& _global, int _button );
  virtual void slotReturnPressed( KIconContainerItem* _item, const QPoint& _global );
  virtual void slotDragStart( const QPoint& _hotspot, QList<KIconContainerItem>& _selected, QPixmap _pixmap );
  virtual void slotDrop( QDropEvent*, KIconContainerItem*, QStrList& _formats );
  
  virtual void slotUpdateError( int _id, int _errid, const char *_errortext );
  virtual void slotUpdateFinished( int _id );
  virtual void slotUpdateListEntry( int _id, UDSEntry& _entry );

  virtual void slotOnItem( KIconContainerItem* );

protected:  
  virtual void initConfig();

  string m_strURL;
  K2URL m_url;
  bool m_bIsLocalURL;
  
  KfmView* m_pView;

  int m_jobId;

  bool m_bComplete;
  string m_strWorkingURL;
  K2URL m_workingURL;

  list<UDSEntry> m_buffer;
  QTimer m_bufferTimer;

  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit;
};

#endif
