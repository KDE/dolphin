/***************************************************************************
 *   Copyright (C) 2009 by Rahman Duran <rahman.duran@gmail.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef DOLPHINREMOTEENCODING_H
#define DOLPHINREMOTEENCODING_H

#include <QStringList>
#include <QtGui/QAction>
#include <kurl.h>


class KActionMenu;
class DolphinViewActionHandler;

/**
 * @brief Allows to chnage character encoding for remote urls like ftp.
 *
 * When browsing remote url, its possible to change encoding from Tools Menu.
 */

class DolphinRemoteEncoding: public QObject
{
  Q_OBJECT
public:
  DolphinRemoteEncoding(QObject* parent, DolphinViewActionHandler* actionHandler);
  ~DolphinRemoteEncoding();

public Q_SLOTS:
  void slotAboutToOpenUrl();
  void slotItemSelected(QAction* action);
  void slotReload();
  void slotDefault();
  
private Q_SLOTS:
  void slotAboutToShow();
  
private:
  void updateView();
  void loadSettings();
  void fillMenu();
  void updateMenu();

  KActionMenu* m_menu;
  QStringList m_encodingDescriptions;
  KUrl m_currentURL;
  DolphinViewActionHandler* m_actionHandler;

  bool m_loaded;
  int m_idDefault;
};

#endif
