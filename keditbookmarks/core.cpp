/* This file is part of the KDE project
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

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

#include <qclipboard.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kaction.h>

#include <dcopref.h>
#include <dcopclient.h>

#include <kwin.h>

#include "core.h"

// DESIGN - remove these

QMimeSource *KEBClipboard::get() {
   return kapp->clipboard()->data(QClipboard::Clipboard);
}

void KEBClipboard::set(QMimeSource *data) {
   kapp->clipboard()->setData(data, QClipboard::Clipboard);
}

void disableDynamicActions(QValueList<KAction *> actions) {
   QValueList<KAction *>::Iterator it = actions.begin();
   QValueList<KAction *>::Iterator end = actions.end();
   for (; it != end; ++it) {
      KAction *act = *it;
      if ((strncmp(act->name(), "options_configure", strlen("options_configure")) != 0)
       && (strncmp(act->name(), "help_", strlen("help_")) != 0)) {
         act->setEnabled(false);
      }
   }
}

void continueInWindow(QString _wname) {
   DCOPClient* dcop = kapp->dcopClient();
   QCString wname = _wname.latin1();
   int id = -1;

   QCStringList apps = dcop->registeredApplications();
   for (QCStringList::Iterator it = apps.begin(); it != apps.end(); ++it) {
      QCString &clientId = *it;

      if (qstrncmp(clientId, wname, wname.length()) != 0) {
         continue;
      }

      DCOPRef client(clientId.data(), wname + "-mainwindow#1");
      DCOPReply result = client.call("getWinID()");

      if (result.isValid()) {
         id = (int)result;
         break;
      }
   }

   KWin::setActiveWindow(id);
}
