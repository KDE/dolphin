/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __dcop_h
#define __dcop_h

#include <kmainwindow.h>
#include <dcopobject.h>
#include <kcommand.h>

#define DCOP_ACCEPT m_modified

// Those methods aren't in KEBTopLevel since KEBTopLevel registers in DCOP
// and we need to create this DCOPObject after registration.
class KBookmarkEditorIface : public QObject, public DCOPObject
{
   Q_OBJECT
   K_DCOP
public:
   KBookmarkEditorIface();
   void connectToToplevel(class KEBTopLevel *top);
k_dcop:
   void slotDcopAddedBookmark2(QString filename, QString url, QString text, QString address, QString icon);
   void slotDcopCreatedNewFolder2(QString filename, QString text, QString address);
signals:
   void addedBookmark(QString filename, QString url, QString text, QString address, QString icon);
   void createdNewFolder(QString filename, QString text, QString address);
};

#endif
