/*
    Copyright (c) 2003 Thiago Macieira <thiago.macieira@kdemail.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License (LGPL) as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later
    version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef REMOTEENCODING_PLUGIN_H
#define REMOTEENCODING_PLUGIN_H

#include <QStringList>
#include <kurl.h>
#include <klibloader.h>
#include <kparts/plugin.h>

class KActionMenu;
class KConfig;
class KonqDirPart;

class KRemoteEncodingPlugin: public KParts::Plugin
{
  Q_OBJECT
public:
  KRemoteEncodingPlugin(QObject * parent, const QStringList &);
  ~KRemoteEncodingPlugin();

protected Q_SLOTS:
  void slotAboutToOpenURL();
  void slotAboutToShow();
  void slotItemSelected(int);
  void slotReload();
  void slotDefault();

private:
  void updateBrowser();
  void loadSettings();
  void fillMenu();
  void updateMenu();

  KonqDirPart *m_part;
  KActionMenu *m_menu;
  QStringList m_encodingDescriptions;
  KUrl m_currentURL;

  bool m_loaded;
  int m_idDefault;
};

#endif
