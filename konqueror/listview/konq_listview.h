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
#include <konqoperations.h>
#include <kparts/factory.h>
#include <konq_dirpart.h>

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
  Q_PROPERTY( bool supportsUndo READ supportsUndo );
public:
  KonqListView( QWidget *parentWidget, QObject *parent, const char *name, const QString& mode );
  virtual ~KonqListView();

  virtual bool openURL( const KURL &url );
  virtual bool closeURL();
  virtual bool openFile() { return true; }

  KonqBaseListViewWidget *listViewWidget() const { return m_pListView; }

  ListViewBrowserExtension *extension() const { return m_browser; }

  bool supportsUndo() const { return true; }

protected:
  virtual void guiActivateEvent( KParts::GUIActivateEvent *event );
  void setupActions();

protected slots:
  void slotSelect();
  void slotUnselect();
  void slotSelectAll();
  void slotUnselectAll();
  void slotInvertSelection();

  void slotIconSizeToggled(bool b);

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
  //and then slotDaveAfterHeaderDrag is called
  void headerDragged(int sec, int from, int to);
  //saves the new order of the columns
  void slotSaveAfterHeaderDrag();
  void slotHeaderClicked(int sec);

private:

  KonqBaseListViewWidget *m_pListView;
  ListViewBrowserExtension *m_browser;

  KAction *m_paSelect;
  KAction *m_paUnselect;
  KAction *m_paSelectAll;
  KAction *m_paUnselectAll;
  KAction *m_paInvertSelection;

  KToggleAction *m_paLargeIcons;
  KToggleAction *m_paMediumIcons;
  KToggleAction *m_paSmallIcons;

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
          m_listView->saveState( stream );
          KParts::BrowserExtension::saveState( stream );
        }

      virtual void restoreState( QDataStream &stream )
        {
          // Note: since restore state currently restores the name filter,
          // which we need BEFORE opening the URL, we do that one first.
          // If we add more stuff to restoreState, we may need to split it
          // into two methods in fact.
          m_listView->restoreState( stream );
          KParts::BrowserExtension::restoreState( stream );
        }

   protected slots:
      void updateActions();

      void copy() { copySelection( false ); }
      void cut() { copySelection( true ); }
      void paste();
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
