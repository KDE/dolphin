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

#include <kbrowser.h>
#include <kuserpaths.h>

#include <qvaluelist.h>
#include <qlistview.h>
#include <qstringlist.h>

class KonqTreeViewWidget;
class KonqTreeView;
class KToggleAction;

/*
class TreeViewPropertiesExtension : public ViewPropertiesExtension
{
  Q_OBJECT
public:
  TreeViewPropertiesExtension( KonqTreeView *treeView );

  virtual void reparseConfiguration();
  virtual void saveLocalProperties();
  virtual void savePropertiesAsDefault();

private:
  KonqTreeView *m_treeView;
};

class TreeViewEditExtension : public EditExtension
{
  Q_OBJECT
public:
  TreeViewEditExtension( KonqTreeView *treeView );

  virtual void can( bool &cut, bool &copy, bool &paste, bool &move );

  virtual void cutSelection();
  virtual void copySelection();
  virtual void pasteSelection( bool move );
  virtual void moveSelection( const QString &destinationURL = QString::null );
  virtual QStringList selectedUrls();

private:
  KonqTreeView *m_treeView;
};
*/
class TreeViewBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  friend class KonqTreeView;
  friend class KonqTreeViewWidget;
public:
  TreeViewBrowserExtension( KonqTreeView *treeView );

  virtual void setXYOffset( int x, int y );
  virtual int xOffset();
  virtual int yOffset();

protected slots:
  void updateActions();

  void copy();
  void cut();
  void del() { moveSelection( QString::null ); }
  void pastecut() { pasteSelection( true ); }
  void pastecopy() { pasteSelection( false ); }
  void trash() { moveSelection( KUserPaths::trashPath() ); }

  void reparseConfiguration();
  void saveLocalProperties();
  void savePropertiesAsDefault();

private:
  void pasteSelection( bool move );
  void moveSelection( const QString &destinationURL );

  KonqTreeView *m_treeView;
};

class KonqTreeView : public KParts::ReadOnlyPart
{
  friend class KonqTreeViewWidget;
  Q_OBJECT
public:
  KonqTreeView( QWidget *parentWidget, QObject *parent, const char *name );
  virtual ~KonqTreeView();

  virtual bool openURL( const KURL &url );

  virtual void closeURL();

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

#endif
