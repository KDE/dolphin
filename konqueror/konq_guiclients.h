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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_guiclients_h__
#define __konq_guiclients_h__

#include <kxmlguiclient.h>
#include <qobject.h>
#include <ktrader.h>

class KAction;
class KActionCollection;
class KonqMainWindow;
class KonqView;

class PopupMenuGUIClient : public KXMLGUIClient
{
public:
  PopupMenuGUIClient( KonqMainWindow *mainWindow, const KTrader::OfferList &embeddingServices,
                      bool dirsSelected );
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

  QList<KAction> actions() const;
  KAction *action( const QString &name ) { return m_actions[ name ]; }

  void saveConfig( bool add, const QString &serviceName );

private slots:
  void slotToggleView( bool toggle );
  void slotViewAdded( KonqView *view );
  void slotViewRemoved( KonqView *view );
private:
  KonqMainWindow *m_mainWindow;
  QDict<KAction> m_actions;
  bool m_empty;
  QMap<QString,bool> m_mapOrientation;
};

#endif
