// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
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
class BookmarkInfoWidget;

struct SelcAbilities {
    bool itemSelected:1;
    bool group:1;
    bool root:1;
    bool separator:1;
    bool urlIsEmpty:1;
    bool multiSelect:1;
    bool singleSelect:1;
    bool notEmpty:1;
    bool tbShowState:1;
};

class CmdHistory : public QObject {
    Q_OBJECT
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

private:
    KCommandHistory m_commandHistory;
    static CmdHistory *s_self;
};

class KBookmark;
class KBookmarkManager;

class CurrentMgr : public QObject {
    Q_OBJECT
public:
    typedef enum {HTMLExport, OperaExport, IEExport, MozillaExport, NetscapeExport} ExportType;

    static CurrentMgr* self() { if (!s_mgr) { s_mgr = new CurrentMgr(); } return s_mgr; }
    static KBookmark bookmarkAt(const QString & a);

    KBookmarkManager* mgr() const { return m_mgr; }
    bool showNSBookmarks() const;
    QString correctAddress(const QString &address) const;
    QString path() const;

    void createManager(const QString &filename);
    void notifyManagers();
    bool managerSave();
    void saveAs(const QString &fileName);
    void doExport(ExportType type, const QString & path = QString::null);
    void setUpdate(bool update);

    void reloadConfig();

protected slots:
    void slotBookmarksChanged(const QString &, const QString &);

private:
    CurrentMgr() : m_mgr(0) { ; }
    KBookmarkManager *m_mgr;
    static CurrentMgr *s_mgr;
};

class KEBApp : public KMainWindow {
    Q_OBJECT
public:
    static KEBApp* self() { return s_topLevel; }

    KEBApp(const QString & bookmarksFile, bool readonly, const QString &address, bool browser, const QString &caption);
    virtual ~KEBApp();

    void setModifiedFlag(bool);

    void updateActions();
    void setActionsEnabled(SelcAbilities);

    void setCancelFavIconUpdatesEnabled(bool);
    void setCancelTestsEnabled(bool);

    void notifyCommandExecuted();

    QWidget* popupMenuFactory(const char *type) { 
        return factory()->container(type, this); 
    }

    KToggleAction* getToggleAction(const char *) const;

    QString caption() const { return m_caption; }
    bool readonly() const { return m_readOnly; }
    bool modified() const { return m_modified; }
    bool splitView() const { return m_splitView; } 
    bool browser() const { return m_browser; } 
    bool nsShown() const;

    BookmarkInfoWidget *bkInfo() { return m_bkinfo; }

public slots:
    void slotSaveOnClose();
    void slotSplitView();

    void slotConfigureToolbars();

protected slots:
    void slotClipboardDataChanged();
    void slotNewToolbarConfig();

private:
    static KBookmarkManager* bookmarkManager();

    virtual bool queryClose();

    void readConfig();
    void resetActions();
    void createActions();

    void updateListView();

    static KEBApp *s_topLevel;
    KBookmarkEditorIface *m_dcopIface;

public: // only temporary
    CmdHistory *m_cmdHistory;
    QString m_bookmarksFilename;
    QString m_caption;

    bool m_modified:1;
    bool m_saveOnClose:1;

    void construct();

private:
    MagicKLineEdit *m_iSearchLineEdit;
    BookmarkInfoWidget *m_bkinfo;

    bool m_canPaste:1;
    bool m_readOnly:1;

    bool m_splitView:1;
    bool m_browser:1;
};

#endif
