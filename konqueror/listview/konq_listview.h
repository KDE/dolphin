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

#ifndef __konq_listview_h__
#define __konq_listview_h__

#include <kparts/browserextension.h>
#include <kglobalsettings.h>
#include <konq_operations.h>
#include <kparts/factory.h>
#include <konq_dirpart.h>
#include <konq_mimetyperesolver.h>

#include <qvaluelist.h>
#include <qlistview.h>
#include <qstringlist.h>

#include <konq_propsview.h>
#include "konq_listviewwidget.h"

class KAction;
class KToggleAction;
class ListViewBrowserExtension;

class KonqListViewFactory : public KParts::Factory
{
public:
  KonqListViewFactory();
  virtual ~KonqListViewFactory();

  virtual KParts::Part* createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList &args );

  static KInstance *instance();
  static KonqPropsView *defaultViewProps();

private:
  static KInstance *s_instance;
  static KonqPropsView *s_defaultViewProps;
};

/**
 * The part for the tree view. It does quite nothing, just the
 * konqueror interface. Most of the functionality is in the
 * widget, KonqListViewWidget.
 */
class KonqListView : public KonqDirPart
{
  friend class KonqBaseListViewWidget;
  Q_OBJECT
  Q_PROPERTY( bool supportsUndo READ supportsUndo )
public:
  KonqListView( QWidget *parentWidget, QObject *parent, const char *name, const QString& mode );
  virtual ~KonqListView();

  virtual bool openURL( const KURL &url );
  virtual bool closeURL();
  virtual bool openFile() { return true; }
  virtual const KFileItem * currentItem();
  virtual KFileItemList selectedFileItems() {return m_pListView->selectedFileItems();};

  KonqBaseListViewWidget *listViewWidget() const { return m_pListView; }

  bool supportsUndo() const { return true; }

  virtual void saveState( QDataStream &stream );
  virtual void restoreState( QDataStream &stream );

  // "Cut" icons : disable those whose URL is in lst, enable the others
  virtual void disableIcons( const KURL::List & lst );

  // See KonqMimeTypeResolver
  void mimeTypeDeterminationFinished() {}
  //int iconSize() { return m_pListView->iconSize(); }
  void determineIcon( KonqBaseListViewItem * item );

  QList<KonqBaseListViewItem> & lstPendingMimeIconItems() { return m_mimeTypeResolver->m_lstPendingMimeIconItems; }
  void listingComplete();

  virtual void newIconSize( int );

protected:
  void setupActions();
  void guiActivateEvent( KParts::GUIActivateEvent *event );

protected slots:
  void slotSelect();
  void slotUnselect();
  void slotSelectAll();
  void slotUnselectAll();
  void slotInvertSelection();
  void slotCaseInsensitive();

  void slotShowDot();
  //this is called if a item in the submenu is toggled
  //it saves the new configuration according to the menu items
  //and calls createColumns()
  //it adjusts the indece of the remaining columns
  void slotColumnToggled();
  //this is called when the user changes the order of the
  //columns by dragging them
  //at this moment the columns haven't changed their order yet, so
  //it starts a singleshottimer, after which the columns changed their order
  //and then slotSaveAfterHeaderDrag is called
  void headerDragged(int sec, int from, int to);
  //saves the new order of the columns
  void slotSaveAfterHeaderDrag();
  void slotHeaderClicked(int sec);

  // This comes from KonqDirPart, it's for the "Find" feature
  virtual void slotStarted() { m_pListView->slotStarted(); }
  virtual void slotCanceled() { m_pListView->slotCanceled(); }
  virtual void slotCompleted() { m_pListView->slotCompleted(); }
  virtual void slotNewItems( const KFileItemList& lst ) { m_pListView->slotNewItems( lst ); }
  virtual void slotDeleteItem( KFileItem * item ) { m_pListView->slotDeleteItem( item ); }
  virtual void slotRefreshItems( const KFileItemList& lst ) { m_pListView->slotRefreshItems( lst ); }
  virtual void slotClear() { m_pListView->slotClear(); }
  virtual void slotRedirection( const KURL & u ) { m_pListView->slotRedirection( u ); }

  // Connected to KonqDirPart
  void slotKFindOpened();
  void slotKFindClosed();

  void slotViewportAdjusted() { m_mimeTypeResolver->slotViewportAdjusted(); }
  void slotProcessMimeIcons() { m_mimeTypeResolver->slotProcessMimeIcons(); }

private:

  KonqBaseListViewWidget *m_pListView;
  KonqMimeTypeResolver<KonqBaseListViewItem,KonqListView> *m_mimeTypeResolver;

  KAction *m_paSelect;
  KAction *m_paUnselect;
  KAction *m_paSelectAll;
  KAction *m_paUnselectAll;
  KAction *m_paInvertSelection;

  KToggleAction *m_paCaseInsensitive;

  KToggleAction *m_paShowDot;
  KToggleAction *m_paShowTime;
  KToggleAction *m_paShowType;
  KToggleAction *m_paShowMimeType;
  KToggleAction *m_paShowAccessTime;
  KToggleAction *m_paShowCreateTime;
  KToggleAction *m_paShowLinkDest;
  KToggleAction *m_paShowSize;
  KToggleAction *m_paShowOwner;
  KToggleAction *m_paShowGroup;
  KToggleAction *m_paShowPermissions;
  KToggleAction *m_paShowURL;
};

class ListViewBrowserExtension : public KParts::BrowserExtension
{
   Q_OBJECT
   friend class KonqListView;
   friend class KonqBaseListViewWidget;
   public:
      ListViewBrowserExtension( KonqListView *listView );

      virtual int xOffset();
      virtual int yOffset();

      virtual void saveState( QDataStream &stream )
        {
          m_listView->saveNameFilter( stream );
          KParts::BrowserExtension::saveState( stream );
          m_listView->saveState( stream );
        }

      virtual void restoreState( QDataStream &stream )
        {
          // Note: since we need to restore the name filter,
          // BEFORE opening the URL.
          m_listView->restoreNameFilter( stream );
          KParts::BrowserExtension::restoreState( stream );
          m_listView->restoreState( stream );
        }

   protected slots:
      void updateActions();

      void copy() { copySelection( false ); }
      void cut() { copySelection( true ); }
      void paste();
      void rename();
      void trash() { KonqOperations::del(m_listView->listViewWidget(),
                                         KonqOperations::TRASH,
                                         m_listView->listViewWidget()->selectedUrls()); }
      void del() { KonqOperations::del(m_listView->listViewWidget(),
                                       KonqOperations::DEL,
                                       m_listView->listViewWidget()->selectedUrls()); }
      void shred() { KonqOperations::del(m_listView->listViewWidget(),
                                         KonqOperations::SHRED,
                                         m_listView->listViewWidget()->selectedUrls()); }

      void reparseConfiguration();
      void setSaveViewPropertiesLocally( bool value );
      void setNameFilter( QString nameFilter );
      // void refreshMimeTypes is missing

      void properties();
      void editMimeType();

   private:
      void copySelection( bool move );

      KonqListView *m_listView;
};

#endif
