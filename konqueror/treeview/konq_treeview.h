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

#include <qvaluelist.h>
#include <qlistview.h>
#include <qstringlist.h>

class KonqTreeViewWidget;
class KonqTreeView;

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
  
private:
  KonqTreeView *m_treeView;
};

class KonqTreeView : public BrowserView
{
  friend class KonqTreeViewWidget;
  friend class TreeViewPropertiesExtension;
  Q_OBJECT
public:
  KonqTreeView();
  virtual ~KonqTreeView();
  
  virtual void openURL( const QString &url, bool reload = false,
                        int xOffset = 0, int yOffset = 0 );

  virtual QString url();
  virtual int xOffset();
  virtual int yOffset();
  virtual void stop();

  KonqTreeViewWidget *treeViewWidget() const { return m_pTreeView; }

protected:
  virtual void resizeEvent( QResizeEvent * );

protected slots:
  void slotReloadTree();
  void slotShowDot();

private:
  KonqTreeViewWidget *m_pTreeView;
  KToggleAction *m_paShowDot;
};

#endif
