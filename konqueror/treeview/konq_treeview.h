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

#ifndef __konq_treeview_h__
#define __konq_treeview_h__

#include <kparts/browserextension.h>
#include <kuserpaths.h>
#include <konqoperations.h>
#include <konq_treeviewwidget.h>

#include <qvaluelist.h>
#include <qlistview.h>
#include <qstringlist.h>

class KToggleAction;
class TreeViewBrowserExtension;

/**
 * The part for the tree view. It does quite nothing, just the
 * konqueror interface. Most of the functionality is in the
 * widget, KonqTreeViewWidget.
 */
class KonqTreeView : public KParts::ReadOnlyPart
{
  friend class KonqTreeViewWidget;
  Q_OBJECT
public:
  KonqTreeView( QWidget *parentWidget, QObject *parent, const char *name );
  virtual ~KonqTreeView();

  virtual bool openURL( const KURL &url );

  virtual bool closeURL();

  virtual bool openFile() { return true; }

  KonqTreeViewWidget *treeViewWidget() const { return m_pTreeView; }

  TreeViewBrowserExtension *extension() const { return m_browser; }

protected:
  virtual void guiActivateEvent( KParts::GUIActivateEvent *event );

protected slots:
  void slotReloadTree();
  void slotShowDot();

private:
  KonqTreeViewWidget *m_pTreeView;
  KToggleAction *m_paShowDot;
  TreeViewBrowserExtension *m_browser;
};

class TreeViewBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  friend class KonqTreeView;
  friend class KonqTreeViewWidget;
public:
  TreeViewBrowserExtension( KonqTreeView *treeView );

  virtual int xOffset();
  virtual int yOffset();

protected slots:
  void updateActions();

  void copy();
  void cut();
  void pastecut() { pasteSelection( true ); }
  void pastecopy() { pasteSelection( false ); }
  void trash() { KonqOperations::del(KonqOperations::TRASH,
                                     m_treeView->treeViewWidget()->selectedUrls()); }
  void del() { KonqOperations::del(KonqOperations::DEL,
                                   m_treeView->treeViewWidget()->selectedUrls()); }
  void shred() { KonqOperations::del(KonqOperations::SHRED,
                                     m_treeView->treeViewWidget()->selectedUrls()); }

  void reparseConfiguration();
  void saveLocalProperties();
  void savePropertiesAsDefault();

private:
  void pasteSelection( bool move );

  KonqTreeView *m_treeView;
};

#endif
