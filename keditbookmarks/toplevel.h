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

#ifndef __toplevel_h
#define __toplevel_h

#include <kmainwindow.h>
#include <kcommand.h>

struct SelcAbilities {
   bool itemSelected:1;
   bool group:1;
   bool root:1;
   bool separator:1;
   bool urlIsEmpty:1;
   bool multiSelect:1;
   bool singleSelect:1;
   bool notEmpty:1;
};

class KBookmarkManager;
class KBookmarkEditorIface;
class ImportCommand;
class KToggleAction;

class KEBApp : public KMainWindow
{
   Q_OBJECT

public:
   static KEBApp* self() { return s_topLevel; }

   QWidget* popupMenuFactory(const char *type) { return factory()->container(type, this); }

   KEBApp(const QString & bookmarksFile, bool readonly, const QString &address);
   virtual ~KEBApp();

   void setModifiedFlag(bool);

   KToggleAction* getToggleAction(const char *);

   void setActionsEnabled(SelcAbilities);

   void setCancelFavIconUpdatesEnabled(bool);
   void setCancelTestsEnabled(bool);

   void emitSlotCommandExecuted();

   void didCommand(KCommand *cmd);
   void addCommand(KCommand *cmd);

   void docSaved();
   void clearHistory();

   void updateActions();

   bool readonly() { return m_readOnly; }
   bool modified() { return m_modified; }
   bool nsShown();

   void setAllOpen(bool open);

private:
   static KBookmarkManager* bookmarkManager();

   bool save();

   void updateListView();

public slots:
   void slotLoad();
   void slotSave();
   void slotSaveAs();
   void slotDocumentRestored();
   void slotSaveOnClose();
   void slotConfigureKeyBindings();
   void slotConfigureToolbars();

protected slots:
   void slotClipboardDataChanged();
   void slotBookmarksChanged(const QString &, const QString &);
   void slotCommandExecuted();
   void slotNewToolbarConfig();

private:
   void construct();
   void resetActions();
   void createActions();

   virtual bool queryClose();

   bool m_modified;
   bool m_canPaste;
   bool m_readOnly;

   KCommandHistory m_commandHistory;
   KBookmarkEditorIface *m_dcopIface;
   QString m_bookmarksFilename;
   bool m_saveOnClose;

   static KEBApp *s_topLevel;
};

#endif
