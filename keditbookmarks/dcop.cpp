// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dcop.h"

#include "toplevel.h"

#include <stdlib.h>

#include <QClipboard>
#include <QPainter>

#include <klocale.h>
#include <kbookmarkmanager.h>
#include <dcopclient.h>
#include <kdebug.h>
#include <kapplication.h>


KBookmarkEditorIface::KBookmarkEditorIface()
    : QObject(), DCOPObject("KBookmarkEditor") {
    // connect(KBookmarkNotifier_stub, SIGNAL( updatedAccessMetadata(QString,QString) ), 
    //         this,                  SLOT( slotDcopUpdatedAccessMetadata(QString,QString) ));
    connectDCOPSignal(0, "KBookmarkNotifier", "updatedAccessMetadata(QString,QString)", "slotDcopUpdatedAccessMetadata(QString,QString)", false);
}

void KBookmarkEditorIface::slotDcopUpdatedAccessMetadata(QString filename, QString url) {
    // evil hack, konqi gets updates by way of historymgr,
    // therefore konqi does'nt want "save" notification,
    // unfortunately to stop konqi getting it is difficult
    // and probably not needed until more notifier events
    // are added. therefore, we always updateaccessmetadata
    // without caring about our modified state like normal
    // and thusly there is no need to the bkmgr to do a "save"

    // TODO - I'm not sure this is really true :)

    if (/*KEBApp::self()->modified() &&*/ filename == CurrentMgr::self()->path()) {
        kDebug() << "slotDcopUpdatedAccessMetadata(" << url << ")" << endl;
        // no undo
        CurrentMgr::self()->mgr()->updateAccessMetadata(url, false);
        //FIXME ListView::self()->updateStatus(url);
        KEBApp::self()->updateStatus(url);
        // notice - no save here! see! :)
    }
}
#include "dcop.moc"
