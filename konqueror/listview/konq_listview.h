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
#include <konq_listviewwidget.h>

#include <qvaluelist.h>
#include <qlistview.h>
#include <qstringlist.h>

#include "konq_listviewwidget.h"

class KAction;
class KToggleAction;
class ListViewBrowserExtension;

/**
 * The part for the tree view. It does quite nothing, just the
 * konqueror interface. Most of the functionality is in the
 * widget, KonqListViewWidget.
 */
class KonqListView : public KParts::ReadOnlyPart
{
  friend class KonqBaseListViewWidget;
  Q_OBJECT
public:
  KonqListView( QWidget *parentWidget, QObject *parent, const char *name, const QString& mode );
  virtual ~KonqListView();

  virtual bool openURL( const KURL &url );
  virtual bool closeURL();
  virtual bool openFile() { return true; }

  KonqBaseListViewWidget *listViewWidget() const { return m_pListView; }

  ListViewBrowserExtension *extension() const { return m_browser; }

protected:
  virtual void guiActivateEvent( KParts::GUIActivateEvent *event );
  void setupActions();

protected slots:
  void slotSelect();
  void slotUnselect();
  void slotSelectAll();
  void slotUnselectAll();
  void slotInvertSelection();

  void slotViewLarge( bool b );
  void slotViewMedium( bool b );
  void slotViewSmall( bool b );
  void slotViewNone( bool b );

  void slotShowDot();
  void slotShowTime();
  void slotShowSize();
  void slotShowOwner();
  void slotShowGroup();
  void slotShowPermissions();

  void slotCheckMimeTypes();
  void slotBackgroundColor();
  void slotBackgroundImage();

  void slotReloadTree();

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
  KToggleAction *m_paNoIcons;

  KToggleAction *m_paShowDot;
  KToggleAction *m_paShowTime;
  KToggleAction *m_paShowSize;
  KToggleAction *m_paShowOwner;
  KToggleAction *m_paShowGroup;
  KToggleAction *m_paShowPermissions;

  KToggleAction *m_paCheckMimeTypes;
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

   protected slots:
      void updateActions();

      void copy();
      void cut();
      void pastecut() { pasteSelection( true ); }
      void pastecopy() { pasteSelection( false ); }
      void trash() { KonqOperations::del(KonqOperations::TRASH,
                                         m_listView->listViewWidget()->selectedUrls()); }
      void del() { KonqOperations::del(KonqOperations::DEL,
                                       m_listView->listViewWidget()->selectedUrls()); }
      void shred() { KonqOperations::del(KonqOperations::SHRED,
                                         m_listView->listViewWidget()->selectedUrls()); }

      void reparseConfiguration();
      void saveLocalProperties();
      void savePropertiesAsDefault();

   private:
      void pasteSelection( bool move );

      KonqListView *m_listView;
};

#endif
