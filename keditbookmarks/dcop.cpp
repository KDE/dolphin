/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
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

#include <stdlib.h>

#include <qclipboard.h>
#include <qpopupmenu.h>
#include <qpainter.h>

#include <klocale.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <kapplication.h>

#include <kaction.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kmessagebox.h>
#include <krun.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkexporter.h>
#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>

#include <klineeditdlg.h>

#include "toplevel.h"
#include "commands.h"
#include "importers.h"
#include "core.h"
#include "favicons.h"
#include "testlink.h"
#include "listview.h"
#include "mymanager.h"

#include "dcop.h"

KBookmarkEditorIface::KBookmarkEditorIface()
 : QObject(), DCOPObject("KBookmarkEditor") {

   connectDCOPSignal(0, "KBookmarkNotifier", "addedBookmark(QString,QString,QString,QString,QString)", "slotDcopAddedBookmark2(QString,QString,QString,QString,QString)", false);
   connectDCOPSignal(0, "KBookmarkNotifier", "createdNewFolder(QString,QString,QString)", "slotDcopCreatedNewFolder2(QString,QString,QString)", false);
}

void KBookmarkEditorIface::connectToToplevel(KEBTopLevel *top) {
   connect(this, SIGNAL( addedBookmark(QString, QString, QString, QString, QString) ),
           top,  SLOT( slotDcopAddedBookmark(QString, QString, QString, QString, QString) ));
   connect(this, SIGNAL( createdNewFolder(QString, QString, QString) ),
           top,  SLOT( slotDcopCreatedNewFolder(QString, QString, QString) ));
}

void KBookmarkEditorIface::slotDcopAddedBookmark2(QString filename, QString url, QString text, QString address, QString icon) {
   emit addedBookmark(filename, url, text, address, icon);
}

void KBookmarkEditorIface::slotDcopCreatedNewFolder2(QString filename, QString text, QString address) {
   emit createdNewFolder(filename, text, address);
}

// DESIGN - can these two be moved out of KEBTopLevel and into "middle layer"
//          in fact, why not remove these extra functions altogether
//          by somehow stringing together the connects?

void KEBTopLevel::slotDcopCreatedNewFolder(QString filename, QString text, QString address) {
   if (DCOP_ACCEPT && filename == MyManager::self()->path()) {
      kdDebug() << "slotDcopCreatedNewFolder(" << text << "," << address << ")" << endl;
      CreateCommand *cmd = new CreateCommand( 
                                  MyManager::self()->correctAddress(address), 
                                  text, QString::null, 
                                  true /*open*/, true /*indirect*/);
      addCommand(cmd);
   }
}

void KEBTopLevel::slotDcopAddedBookmark(QString filename, QString url, QString text, QString address, QString icon) {
   if (DCOP_ACCEPT && filename == MyManager::self()->path()) {
      kdDebug() << "slotDcopAddedBookmark(" << url << "," << text << "," << address << "," << icon << ")" << endl;
      CreateCommand *cmd = new CreateCommand(
                                  MyManager::self()->correctAddress(address), 
                                  text, icon, KURL(url), true /*indirect*/);
      addCommand(cmd);
   }
}

#include "dcop.moc"
