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
#include <konq_fileitem.h>
#include <klistview.h>
#include <kparts/browserextension.h>

#include <konq_propsview.h>
namespace KIO { class Job; }
class QCursor;
class KonqDirLister;
class KonqBaseListViewItem;
class KonqFMSettings;
class ListViewPropertiesExtension;
class KToggleAction;

class ColumnInfo
{
   public:
      ColumnInfo();
      ColumnInfo(const char* n,const char* desktopName,int kioUds,int count,bool enabled,KToggleAction* someAction);
      void setData(const char* n,const char* desktopName,int kioUds,int count,bool enabled,KToggleAction* someAction);
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
      enum {NumberOfAtoms=11};

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
      KonqPropsView * props() const;

      //QList<ColumnInfo> *columnConfigInfo() {return &confColumns;};
      ColumnInfo * columnConfigInfo() {return confColumns;};
      QString sortedByColumn;
      bool ascending;

      virtual void setShowIcons( bool enable ) { m_showIcons = enable; }
      virtual bool showIcons() { return m_showIcons; }

      void setItemFont( const QFont &f ) { m_itemFont = f; }
      QFont itemFont() const { return m_itemFont; }
      void setItemColor( const QColor &c ) { m_itemColor = c; }
      QColor itemColor() const { return m_itemColor; }
      int iconSize() const { return props()->iconSize(); }

      virtual void paintEmptyArea( QPainter *p, const QRect &r );

      virtual void saveState( QDataStream & ) {}
      virtual void restoreState( QDataStream & ) {}

      virtual void disableIcons( const QStrList & lst );

   public slots:
      //virtual void slotOnItem( KonqBaseListViewItem* _item );
      virtual void updateSelectedFilesInfo();
      void slotMouseButtonPressed(int _button, QListViewItem* _item, const QPoint&, int col);
      virtual void slotOnItem( QListViewItem* _item );
      virtual void slotOnViewport();
      virtual void slotExecuted( QListViewItem* _item );

   protected slots:
      // from QListView
      virtual void slotReturnPressed( QListViewItem *_item );
      virtual void slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column );

      virtual void slotCurrentChanged( QListViewItem* _item ) { slotOnItem( _item ); }

      void slotOpenURLRequest();

      // slots connected to the directory lister
      virtual void slotStarted( const QString & );
      virtual void slotCompleted();
      virtual void slotCanceled();
      virtual void slotClear();
      virtual void slotNewItems( const KFileItemList & );
      virtual void slotDeleteItem( KFileItem * );
      virtual void slotRefreshItems( const KFileItemList & );
      virtual void slotRedirection( const KURL & );
      virtual void slotCloseView();
      void slotPopupMenu(KListView* , QListViewItem* );

   protected:
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
      //QStringList readProtocolConfig( const QString & protocol );

      virtual void startDrag();
      virtual void viewportDragMoveEvent( QDragMoveEvent *_ev );
      virtual void viewportDragEnterEvent( QDragEnterEvent *_ev );
      virtual void viewportDragLeaveEvent( QDragLeaveEvent *_ev );
      virtual void viewportDropEvent( QDropEvent *_ev );

      /** Common method for slotCompleted and slotCanceled */
      virtual void setComplete();

      virtual void popupMenu( const QPoint& _global );

      virtual bool isSingleClickArea( const QPoint& _point );

      virtual void drawContentsOffset( QPainter*, int _offsetx, int _offsety,
                                   int _clipx, int _clipy,
                                   int _clipw, int _cliph );

      //this one is called only by KListView, and this is friend anyways (Alex)
      //KonqDirLister *dirLister() const { return m_dirLister; }

      /** The directory lister for this URL */
      KonqDirLister* m_dirLister;

      //QList<ColumnInfo> confColumns;
      // IMO there is really no need for an advanced data structure
      //we have a fixed number of members,
      //it consumes less memory and access should be faster (Alex)
      ColumnInfo confColumns[NumberOfAtoms];
      //maybe I can do some speedup...
      //ColumnInfo* orderOfColumns[NumberOfAtoms];

      KonqFileItem * openURLRequestFileItem;
      KonqBaseListViewItem* m_dragOverItem;
    //QStringList m_lstDropFormats;

      QFont m_itemFont;
      QColor m_itemColor;

      KonqFMSettings* m_pSettings;

      bool m_bTopLevelComplete;

      bool m_filesSelected;
      bool m_showIcons;
      int m_filenameColumn;
      bool m_bUpdateContentsPosAfterListing;

      KURL m_url;

   public:
      KonqListView *m_pBrowserView;
   protected:
      QString m_selectedFilesStatusText;
};

#endif
