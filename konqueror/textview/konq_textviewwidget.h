/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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
#ifndef __konq_textviewwidget_h__
#define __konq_textviewwidget_h__

#include <qlistview.h>
#include <qcursor.h>
#include <kurl.h>
#include <konqfileitem.h>

#include <qdatetime.h>

#define REGULAR 0
#define REGULARLINK 1
#define EXEC 2
#define DIR 3
#define DIRLINK 4
#define BADLINK 5
#define SOCKET 6
#define CHARDEV 7
#define BLOCKDEV 8
#define FIFO 9
#define UNKNOWN 10

namespace KIO { class Job; }
struct KUDSAtom;
class QCursor;
class KDirLister;
class KonqTextViewItem;
class KonqTextViewDir;
class KonqTextView;
class KonqPropsView;
class KonqFMSettings;
class TextViewPropertiesExtension;

/**
 * The text view
 */
class KonqTextViewWidget : public QListView
{
   friend KonqTextViewItem;
   friend KonqTextViewDir;
   friend KonqTextView;
   friend class TextViewBrowserExtension;

   Q_OBJECT
   public:
      KonqTextViewWidget( KonqTextView *parent, QWidget *parentWidget );
      ~KonqTextViewWidget();

      void stop();
      const KURL & url();

      struct iterator
      {
         KonqTextViewItem* m_p;

         iterator() : m_p( 0L ) { }
         iterator( KonqTextViewItem* _b ) : m_p( _b ) { }
         iterator( const iterator& _it ) : m_p( _it.m_p ) { }

         KonqTextViewItem& operator*() { return *m_p; }
         KonqTextViewItem* operator->() { return m_p; }
         bool operator==( const iterator& _it ) { return ( m_p == _it.m_p ); }
         bool operator!=( const iterator& _it ) { return ( m_p != _it.m_p ); }
         iterator& operator++();
         iterator operator++(int);
      };
      iterator begin() { iterator it( (KonqTextViewItem*)firstChild() ); return it; }
      iterator end() { iterator it; return it; }

      virtual bool openURL( const KURL &url );

      void setXYOffset( int x, int y ) { m_iXOffset = x; m_iYOffset = y; }

      virtual void selectedItems( QValueList<KonqTextViewItem*>& _list );
      KURL::List selectedUrls();

      /**
       * @return the Properties instance for this view. Used by the items.
       */
      KonqPropsView * props() { return m_pProps; }
      void updateSelectedFilesInfo();

   public slots:
      virtual void slotOnItem( KonqTextViewItem* _item );

   protected slots:
      virtual void slotReturnPressed( QListViewItem *_item );
      virtual void slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column );

      // slots connected to the directory lister
      virtual void slotResult(KIO::Job * job);
      virtual void slotStarted( const QString & );
      virtual void slotCompleted();
      virtual void slotCanceled();
      virtual void slotClear();
      virtual void slotNewItems( const KonqFileItemList & );
      virtual void slotDeleteItem( KonqFileItem * );

      virtual void slotCurrentChanged( QListViewItem* _item ) { slotOnItem( (KonqTextViewItem*)_item ); }

   protected:
      virtual void initConfig();
      virtual void viewportDragMoveEvent( QDragMoveEvent *_ev );
      virtual void viewportDragEnterEvent( QDragEnterEvent *_ev );
      virtual void viewportDragLeaveEvent( QDragLeaveEvent *_ev );
      virtual void viewportDropEvent( QDropEvent *_ev );

      virtual void viewportMousePressEvent( QMouseEvent *_ev );
      virtual void viewportMouseMoveEvent( QMouseEvent *_ev );
      virtual void viewportMouseReleaseEvent( QMouseEvent *_ev );
      virtual void keyPressEvent( QKeyEvent *_ev );

      /** Common method for slotCompleted and slotCanceled */
      virtual void setComplete();

      virtual void popupMenu( const QPoint& _global );

      virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety,
                                       int _clipx, int _clipy,
                                       int _clipw, int _cliph );

      virtual void focusInEvent( QFocusEvent* _event );

      KDirLister *dirLister() const { return m_dirLister; }

      /** The directory lister for this URL */
      KDirLister* m_dirLister;

      /** Konqueror settings */
      KonqFMSettings * m_pSettings;

      /** View properties */
      KonqPropsView * m_pProps;

      bool m_bTopLevelComplete;

      KonqTextViewItem* m_dragOverItem;
      KonqTextViewItem* m_overItem;
      QStringList m_lstDropFormats;

      bool m_pressed;
      QPoint m_pressedPos;
      KonqTextViewItem* m_pressedItem;

      QCursor m_stdCursor;
      QCursor m_handCursor;
      QPixmap m_bgPixmap;

      // TODO remove this and use KonqFMSettings
      bool m_bSingleClick;
      bool m_bUnderlineLink;
      bool m_bChangeCursor;

      int m_iXOffset;
      int m_iYOffset;

      long int m_idShowDot;

      KURL m_sURL;

      KonqTextView *m_pBrowserView;
      //this timer is only for testing, can be reomved
      QTime timer;
      
      KLocale *locale;
      QColor colors[11];
      QColor highlight[11];

      bool m_showTime;
      bool m_showSize;
      bool m_showOwner;
      bool m_showGroup;
      bool m_showPermissions;
      bool m_settingsChanged;
      bool m_filesSelected;
      QString m_selectedFilesStatusText;
};

#endif
