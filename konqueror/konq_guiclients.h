/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_guiclients_h__
#define __konq_guiclients_h__

#include <kxmlguiclient.h>
#include <QObject>
#include <QHash>
#include <ktrader.h>

class KAction;
class KActionCollection;
class KonqMainWindow;
class KonqView;

/**
 * This XML-GUI-Client is passed to KonqPopupMenu to add extra actions into it,
 * using the XMLGUI merging. It offers embedding actions and tabbed-browsing actions.
 * Its XML looks like this:
 * @code

 <kpartgui name="konqueror" >
 <Menu name="popupmenu" >
  <menu group="preview" name="preview submenu" >
   <text>Preview In</text>
   <action group="preview" name="0" />
   <action group="preview" name="1" />
  </menu>
  <action group="tabhandling" name="sameview" />
  <action group="tabhandling" name="newview" />
  <action group="tabhandling" name="openintab" />
  <separator group="tabhandling" />
 </Menu>
 </kpartgui>

 * @endcode
 */
class PopupMenuGUIClient : public KXMLGUIClient
{
public:
  PopupMenuGUIClient( KonqMainWindow *mainWindow, const KTrader::OfferList &embeddingServices,
                      bool isIntoTrash, bool doTabHandling );
  virtual ~PopupMenuGUIClient();

  virtual KAction *action( const QDomElement &element ) const;

private:
  void addEmbeddingService( QDomElement &menu, int idx, const QString &name, const KService::Ptr &service );

  KonqMainWindow *m_mainWindow;

  QDomDocument m_doc;
};

class ToggleViewGUIClient : public QObject
{
  Q_OBJECT
public:
  ToggleViewGUIClient( KonqMainWindow *mainWindow );
  virtual ~ToggleViewGUIClient();

  bool empty() const { return m_empty; }

  QList<KAction*> actions() const;
  KAction *action( const QString &name ) { return m_actions[ name ]; }

  void saveConfig( bool add, const QString &serviceName );

private Q_SLOTS:
  void slotToggleView( bool toggle );
  void slotViewAdded( KonqView *view );
  void slotViewRemoved( KonqView *view );
private:
  KonqMainWindow *m_mainWindow;
  QHash<QString,KAction*> m_actions;
  bool m_empty;
  QMap<QString,bool> m_mapOrientation;
};

#endif
