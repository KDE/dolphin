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
#include <kbookmark.h>

class KBookmarkManager;
class KToggleAction;
class KBookmarkEditorIface;
class ImportCommand;
class MagicKLineEdit;

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

class CmdHistory : public QObject {
   Q_OBJECT
private:
   KCommandHistory m_commandHistory;
   static CmdHistory *s_self;
public:
   CmdHistory(KActionCollection *collection);
   virtual ~CmdHistory() { ; }
   void notifyDocSaved();
   void clearHistory();
   void addCommand(KCommand *);
   void didCommand(KCommand *);
   static CmdHistory *self();
protected slots:
   void slotCommandExecuted();
   void slotDocumentRestored();
};

class KBookmark;
class KBookmarkManager;

class CurrentMgr : public QObject {
   Q_OBJECT
public:
   static CurrentMgr* self() { if (!s_mgr) { s_mgr = new CurrentMgr(); } return s_mgr; }
   void createManager(const QString &filename);
   typedef enum {HTMLExport, OperaExport, IEExport, MozillaExport, NetscapeExport} ExportType;
   void doExport(ExportType type);
   void notifyManagers();
   QString correctAddress(const QString &address);
   KBookmarkManager* mgr() const { return m_mgr; }
   static KBookmark bookmarkAt(const QString & a);
   bool managerSave();
   void saveAs(const QString &fileName);
   void setUpdate(bool update);
   QString path();
   bool showNSBookmarks();
protected slots:
   void slotBookmarksChanged(const QString &, const QString &);
private:
   CurrentMgr() : m_mgr(0) { ; }
   KBookmarkManager *m_mgr;
   static CurrentMgr *s_mgr;
};

class BookmarkInfoWidget : public QWidget {
public:
   BookmarkInfoWidget(QWidget * = 0, const char * = 0);
public:
   void showBookmark(const KBookmark &bk);
private:
   KLineEdit *m_title_le, *m_url_le, *m_comment_le, *m_moddate_le, *m_credate_le;
   KBookmark m_bk;
};

class KEBApp : public KMainWindow
{
   Q_OBJECT

public:
   static KEBApp* self() { return s_topLevel; }

   QWidget* popupMenuFactory(const char *type) { return factory()->container(type, this); }

   KEBApp(const QString & bookmarksFile, bool readonly, const QString &address, bool browser);
   virtual ~KEBApp();

   void setModifiedFlag(bool);

   KToggleAction* getToggleAction(const char *);

   void setActionsEnabled(SelcAbilities);

   void setCancelFavIconUpdatesEnabled(bool);
   void setCancelTestsEnabled(bool);

   void notifyCommandExecuted();

   void updateActions();

   bool readonly() { return m_readOnly; }
   bool modified() { return m_modified; }
   bool nsShown();

   BookmarkInfoWidget *bkInfo() { return m_bkinfo; }

private:
   static KBookmarkManager* bookmarkManager();

   bool save();

   void updateListView();

public slots:
   void slotLoad();
   void slotSave();
   void slotSaveAs();
   void slotSaveOnClose();
   void slotAdvancedAddBookmark();
   void slotConfigureKeyBindings();
   void slotConfigureToolbars();

protected slots:
   void slotClipboardDataChanged();
   void slotNewToolbarConfig();

private:
   void construct();
   void readConfig();
   void resetActions();
   void createActions();

   virtual bool queryClose();

   bool m_modified;
   bool m_canPaste;
   bool m_readOnly;

   CmdHistory *m_cmdHistory;
   MagicKLineEdit *m_iSearchLineEdit;
   KBookmarkEditorIface *m_dcopIface;
   QString m_bookmarksFilename;
   bool m_saveOnClose;
   bool m_advancedAddBookmark;
   bool m_browser;

   static KEBApp *s_topLevel;

   BookmarkInfoWidget *m_bkinfo;
};

#endif
