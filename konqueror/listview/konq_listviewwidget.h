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
#ifndef __konq_listviewwidget_h__
#define __konq_listviewwidget_h__

#include <qcursor.h>
#include <qpixmap.h>
#include <qintdict.h>
#include <qtimer.h>
#include <kurl.h>
#include <konqfileitem.h>
#include <klistview.h>
#include <kparts/browserextension.h>

namespace KIO { class Job; }
class QCursor;
class KonqDirLister;
class KonqBaseListViewItem;
class KonqPropsView;
class KonqFMSettings;
class ListViewPropertiesExtension;

class ColumnInfo
{
   public:
      ColumnInfo(const char* n,const char* desktopName,int kioUds,int count,bool enabled,KToggleAction* someAction);
      int displayInColumn;
      QString name;
      QString desktopFileName;
      int udsId;
      bool displayThisOne;
      KToggleAction *toggleThisOne;
};

/**
 * The tree view widget (based on KListView).
 * Most of the functionality is here.
 */
class KonqBaseListViewWidget : public KListView
{
   friend class KonqListView;
   friend class ListViewBrowserExtension;

   Q_OBJECT
   public:
      KonqBaseListViewWidget( KonqListView *parent, QWidget *parentWidget);
      virtual ~KonqBaseListViewWidget();


      virtual void stop();
      const KURL & url();

      struct iterator
      {
         KonqBaseListViewItem* m_p;

         iterator() : m_p( 0L ) { }
         iterator( KonqBaseListViewItem* _b ) : m_p( _b ) { }
         iterator( const iterator& _it ) : m_p( _it.m_p ) { }

         KonqBaseListViewItem& operator*() { return *m_p; }
         KonqBaseListViewItem* operator->() { return m_p; }
         bool operator==( const iterator& _it ) { return ( m_p == _it.m_p ); }
         bool operator!=( const iterator& _it ) { return ( m_p != _it.m_p ); }
         iterator& operator++();
         iterator operator++(int);
      };
      iterator begin() { iterator it( (KonqBaseListViewItem*)firstChild() ); return it; }
      iterator end() { iterator it; return it; }

      virtual bool openURL( const KURL &url );

      void selectedItems( QValueList<KonqBaseListViewItem*>& _list );
      KURL::List selectedUrls();

      /** @return the KonqListViewDir which handles the directory _url */
      //virtual KonqListViewDir * findDir ( const QString & _url );

      /**
       * @return the Properties instance for this view. Used by the items.
       */
      KonqPropsView * props() { return m_pProps; }

      QList<ColumnInfo> *columnConfigInfo() {return &confColumns;};

      virtual void setCheckMimeTypes( bool enable ) { m_checkMimeTypes = enable; }
      virtual bool checkMimetypes() { return m_checkMimeTypes; }

      virtual void setShowIcons( bool enable ) { m_showIcons = enable; }
      virtual bool showIcons() { return m_showIcons; }
      virtual void updateSelectedFilesInfo();

//      bool underlineLink()            {return m_bUnderlineLink;}
//      bool singleClick()              {return m_bSingleClick;}

      void setItemFont( const QFont &f ) { m_itemFont = f; }
      QFont itemFont() const { return m_itemFont; }
      void setItemColor( const QColor &c ) { m_itemColor = c; }
      QColor itemColor() const { return m_itemColor; }
      void setColor( const QColor &c ) { m_color = c; }
      QColor color() const { return m_color; }
      int iconSize() const {return m_iconSize;};

   public slots:
      //virtual void slotOnItem( KonqBaseListViewItem* _item );
      virtual void slotOnItem( QListViewItem* _item );
      virtual void slotOnViewport();
      virtual void slotExecuted( QListViewItem* _item );

   protected slots:
      // from QListView
      virtual void slotReturnPressed( QListViewItem *_item );
      virtual void slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column );

      virtual void slotCurrentChanged( QListViewItem* _item ) { slotOnItem( _item ); }

      // slots connected to the directory lister
      virtual void slotStarted( const QString & );
      virtual void slotCompleted();
      virtual void slotCanceled();
      virtual void slotClear();
      virtual void slotNewItems( const KFileItemList & );
      virtual void slotDeleteItem( KFileItem * );
      virtual void slotRefreshItems( const KFileItemList & );
      virtual void slotRedirection( const KURL & );

   protected:
      /* Completely custom keyboard selection style:
       home: move to the first
       end: move to the last
       PgUp/PgDn: move one page up/down
       up/down: move one item up/down
       insert: toggle selection of current and move to the next
       space: toggle selection of the current
       SHIFT+CTRL+up: move to the previous item and toggle selection of this one
       SHIFT+CTRL+down: toggle selection of the current item and move to the next
       SHIFT+CTRL+end: toggle selection from (including) the current
       item to (including) the last item
       SHIFT+CTRL+home: toggle selection from (including) the current
       item to the (including) the first item
       SHIFT+CTRL+PgDn: toggle selection from (including) the current
       item to (excluding) the item one page down
       SHIFT+CTRL+PgUp: toggle selection from (excluding) the current
       item to (including) the item one page up

       the combinations work the same with SHIFT instead of CTRL, except
       that if you start selecting something using SHIFT everything selected
       before will be deselected first

       This way e.g. SHIFT+up/PgUp then SHIFT+down/PgDn leaves no item selected
       */
      virtual void keyPressEvent( QKeyEvent *_ev );

      //creates the listview columns according to confColumns
      virtual void createColumns();
      //reads the configuration for the columns of the current
      //protocol, it is called when the protocol changes
      //it checks/unchecks the menu items and sets confColumns
      void readProtocolConfig( const QString & protocol );
      //calls updateContents of every ListViewItem, called after
      //the columns changed
      void updateListContents();

      //this is called in the constructor, so virtual would be nonsense
      void initConfig();
      void emitOpenURLRequest(const KURL& url, const KParts::URLArgs& args);
      void emitStarted( KIO::Job* job);
      void emitCompleted();
      //QStringList readProtocolConfig( const QString & protocol );

      virtual void viewportDragMoveEvent( QDragMoveEvent *_ev );
      virtual void viewportDragEnterEvent( QDragEnterEvent *_ev );
      virtual void viewportDragLeaveEvent( QDragLeaveEvent *_ev );
      virtual void viewportDropEvent( QDropEvent *_ev );

      virtual void viewportMousePressEvent( QMouseEvent *_ev );
      virtual void viewportMouseMoveEvent( QMouseEvent *_ev );
      virtual void viewportMouseReleaseEvent( QMouseEvent *_ev );
      //virtual void keyPressEvent( QKeyEvent *_ev );

      /** Common method for slotCompleted and slotCanceled */
      virtual void setComplete();

      virtual void popupMenu( const QPoint& _global );

      virtual bool isSingleClickArea( const QPoint& _point );

      virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety,
				   int _clipx, int _clipy,
				   int _clipw, int _cliph );

      virtual void focusInEvent( QFocusEvent* _event );

      KonqDirLister *dirLister() const { return m_dirLister; }

      /** The directory lister for this URL */
      KonqDirLister* m_dirLister;

      /** View properties */
      KonqPropsView * m_pProps;

      //I think I could use a simple *array for this... (Alex)
      QList<ColumnInfo> confColumns;

      KonqBaseListViewItem* m_dragOverItem;
      QStringList m_lstDropFormats;

      bool m_pressed;
      QPoint m_pressedPos;
      KonqBaseListViewItem* m_pressedItem;

      QPixmap m_bgPixmap;
      QFont m_itemFont;
      QColor m_itemColor;
      QColor m_color;
      int m_iconSize;

      KonqFMSettings* m_pSettings;

      bool m_bTopLevelComplete;

      long int m_idShowDot;

      bool m_filesSelected;
      bool m_checkMimeTypes;
      bool m_showIcons;
      int m_filenameColumn;

      KURL m_url;

   public:
      KonqListView *m_pBrowserView;
   protected:
      QString m_selectedFilesStatusText;
      bool m_wasShiftEvent;
};

#endif
