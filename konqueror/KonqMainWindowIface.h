/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#ifndef __KonqMainWindowIface_h__
#define __KonqMainWindowIface_h__

#include <dcopobject.h>
#include <qvaluelist.h>
#include <dcopref.h>

class KonqMainWindow;
class KDCOPActionProxy;

/**
 * DCOP interface for a konqueror main window
 */
class KonqMainWindowIface : virtual public DCOPObject
{
  K_DCOP
public:

  KonqMainWindowIface( KonqMainWindow * mainWindow );
  ~KonqMainWindowIface();

k_dcop:

  void openURL( QString url );

  int viewCount();

  int activeViewsCount();

  /**
   * @return reference to the current KonqView
   */
  DCOPRef currentView();
  /**
   * @return reference to the current part
   */
  DCOPRef currentPart();

  DCOPRef action( const QCString &name );
  QCStringList actions();
  QMap<QCString,DCOPRef> actionMap();

private:

  KonqMainWindow * m_pMainWindow;
  KDCOPActionProxy *m_dcopActionProxy;

};

#endif

