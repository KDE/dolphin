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

// #define DEBUG_ADDRESSES

#define COL_ADDR 3
#define COL_STAT 2

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

class QListViewItem;
class KBookmark;
class KBookmarkManager;
class KListView;
class KEBListView;
class KEBListViewItem;
class BookmarkIterator;
class ImportCommand;
class KBookmarkEditorIface;
class MyManager;

class KEBTopLevel : public KMainWindow
{
   Q_OBJECT

public:
   static KEBTopLevel* self() { return s_topLevel; }

   QWidget* popupMenuFactory(const char *type) { return factory()->container(type, this); }

   KEBTopLevel(const QString & bookmarksFile, bool readonly);
   virtual ~KEBTopLevel();

   void setModifiedFlag(bool);

   void setActionsEnabled(SelcAbilities);

   void setCancelFavIconUpdatesEnabled(bool);
   void setCancelSearchEnabled(bool);
   void setCancelTestsEnabled(bool);

   void emitSlotCommandExecuted();

   void addImport(ImportCommand *cmd);

   void didCommand(KCommand *cmd);
   void addCommand(KCommand *cmd);

   void docSaved();
   void clearHistory();

   void updateActions();

   bool readonly() { return m_readOnly; }

private:
   static KBookmarkManager* bookmarkManager();

   bool save();

   void updateListView();

public slots:
   void slotImportKDE();
   void slotImportGaleon();
   void slotImportOpera();
   void slotImportIE();
   void slotImportNS();
   void slotExportNS();
   void slotImportMoz();
   void slotExportMoz();
   void slotLoad();
   void slotSave();
   void slotSaveAs();
   void slotDocumentRestored();
   void slotCut();
   void slotCopy();
   void slotPaste();
   void slotRename();
   void slotChangeURL();
   void slotChangeIcon();
   void slotDelete();
   void slotNewFolder();
   void slotNewBookmark();
   void slotInsertSeparator();
   void slotSort();
   void slotSetAsToolbar();
   void slotOpenLink();
   void slotShowNS();
   void slotSaveOnClose();
   void slotConfigureKeyBindings();
   void slotConfigureToolbars();
   void slotTestSelection();
   void slotTestAll();
   void slotCancelAllTests();
   void slotUpdateFavIcon();
   void slotUpdateAllFavIcons();
   void slotCancelFavIconUpdates();
   void slotSearch();
   void slotCancelSearch();
   void slotExpandAll();
   void slotCollapseAll();

protected slots:
   void slotClipboardDataChanged();
   void slotBookmarksChanged(const QString &, const QString &);
   void slotCommandExecuted();
   void slotNewToolbarConfig();
   void slotDcopAddedBookmark(QString, QString, QString, QString, QString);
   void slotDcopCreatedNewFolder(QString, QString, QString);

private:
   void construct();

   void resetActions();
   void createActions();
   virtual bool queryClose();
   void setAllOpen(bool open);

   bool m_modified;
   bool m_canPaste;
   bool m_readOnly;

   KCommandHistory m_commandHistory;
   KBookmarkEditorIface *m_dcopIface;
   QString m_bookmarksFilename;
   bool m_saveOnClose;

   static KEBTopLevel *s_topLevel;

};

#endif
