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

#ifndef __actionsimpl_h
#define __actionsimpl_h

#include <qobject.h>

class ActionsImpl : public QObject
{
   Q_OBJECT

public slots:
   void slotCut();
   void slotCopy();
   void slotPaste();
   void slotRename();
   void slotChangeURL();
   void slotChangeComment();
   void slotChangeIcon();
   void slotDelete();
   void slotNewFolder();
   void slotNewBookmark();
   void slotInsertSeparator();
   void slotSort();
   void slotSetAsToolbar();
   void slotOpenLink();
   void slotShowNS();
   void slotTestSelection();
   void slotTestAll();
   void slotCancelAllTests();
   void slotUpdateFavIcon();
   void slotRecursiveSort();
   void slotUpdateAllFavIcons();
   void slotCancelFavIconUpdates();
   void slotExpandAll();
   void slotCollapseAll();
   void slotImport();
   void slotExportOpera();
   void slotExportHTML();
   void slotExportIE();
   void slotExportNS();
   void slotExportMoz();

   static ActionsImpl* self() { if (!s_self) { s_self = new ActionsImpl(); }; return s_self; }
private:
   ActionsImpl() { ; }
   static ActionsImpl *s_self;
};

#endif
