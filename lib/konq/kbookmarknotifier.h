/* This file is part of the KDE project
   Copyright (C) 2001 Alexander Kellett <lypanov@gmx.net>

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

#ifndef __kbookmarknotifier_h__
#define __kbookmarknotifier_h__

#include <dcopobject.h>

/**
 * DCOP interface for a bookmark listener
 */
class KBookmarkNotifier : public DCOPObject
{
  K_DCOP
public:
  // TODO KBookmarkNotifier() : DCOPObject("KBookmarkNotifier") {}

k_dcop_signals:
  void addBookmark_signal( QString url, QString text, QString address, QString icon );
  void createNewFolder_signal( QString text, QString address );
};

#endif

