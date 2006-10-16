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

#ifndef __toplevel_h
#define __toplevel_h

#include <kmainwindow.h>
#include <kcommand.h>
#include <kbookmark.h>
#include <QMenu>
#include <kxmlguifactory.h>
#include "bookmarklistview.h"

class KBookmarkManager;
class KToggleAction;
class KBookmarkEditorIface;
class ImportCommand;
class BookmarkInfoWidget;
class IKEBCommand;
class BookmarkModel;
class BookmarkListView;

class CmdHistory : public QObject {
    Q_OBJECT
public:
    CmdHistory(KActionCollection *collection);
    virtual ~CmdHistory() { ; }

    void notifyDocSaved();

    void clearHistory();
    void addCommand(KCommand *);
    void didCommand(KCommand *);
    
    //For an explanation see bookmarkInfo::commitChanges()
    void addInFlightCommand(KCommand *);

    static CmdHistory *self();

protected Q_SLOTS:
    void slotCommandExecuted(KCommand *k);

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
    KBookmarkGroup root();
    static KBookmark bookmarkAt(const QString & a);

    KBookmarkManager* mgr() const { return m_mgr; }
    QString path() const;

    void createManager(const QString &filename, const QString &dbusObjectName);
    void notifyManagers(KBookmarkGroup grp);
    void notifyManagers();
    bool managerSave();
    void saveAs(const QString &fileName);
    void doExport(ExportType type, const QString & path = QString());
    void setUpdate(bool update);

    void reloadConfig();

    static QString makeTimeStr(const QString &);
    static QString makeTimeStr(int);

protected Q_SLOTS:
    void slotBookmarksChanged(const QString &, const QString &);

private:
    CurrentMgr() : m_mgr(0), ignorenext(0) { ; }
    KBookmarkManager *m_mgr;
    static CurrentMgr *s_mgr;
    uint ignorenext;
};

class KEBApp : public KMainWindow {
    Q_OBJECT
public:
    static KEBApp* self() { return s_topLevel; }

    KEBApp(const QString & bookmarksFile, bool readonly, const QString &address, bool browser, const QString &caption, const QString& dbusObjectName);
    virtual ~KEBApp();

    void reset(const QString & caption, const QString & bookmarksFileName);

    void updateActions();
    void updateStatus(QString url);
    void setActionsEnabled(SelcAbilities);

    void setCancelFavIconUpdatesEnabled(bool);
    void setCancelTestsEnabled(bool);

    void notifyCommandExecuted();
    void findURL(QString url);

    QMenu* popupMenuFactory(const char *type)
    {
        QWidget * menu = factory()->container(type, this);
        return dynamic_cast<QMenu *>(menu);
    }

    KToggleAction* getToggleAction(const char *) const;

    QString caption() const { return m_caption; }
    bool readonly() const { return m_readOnly; }
    bool browser() const { return m_browser; } 
    bool nsShown() const;

    BookmarkInfoWidget *bkInfo() { return m_bkinfo; }

    void collapseAll();
    void expandAll();

    enum Column { 
      NameColumn = 0,
      UrlColumn = 1,
      CommentColumn = 2,
      StatusColumn = 3
    };
    void startEdit( Column c );
    KBookmark firstSelected() const;
    QString insertAddress() const;
    KBookmark::List selectedBookmarks() const;
    KBookmark::List selectedBookmarksExpanded() const;
    KBookmark::List allBookmarks() const
        { return KBookmark::List();} //FIXME look up what it is suppposed to do, seems like only bookmarks but not folder are returned
public Q_SLOTS:
    void slotConfigureToolbars();

protected Q_SLOTS:
    void slotClipboardDataChanged();
    void slotNewToolbarConfig();
    void selectionChanged();

private:
    void selectedBookmarksExpandedHelper(KBookmark bk, KBookmark::List & bookmarks) const;
    void collapseAllHelper( QModelIndex index );
    void expandAllHelper(QTreeView * view, QModelIndex index);

public: //FIXME
    BookmarkListView * mBookmarkListView;
    BookmarkFolderView * mBookmarkFolderView;
private:

    void resetActions();
    void createActions();

    static KEBApp *s_topLevel;
    KBookmarkEditorIface *m_dcopIface;

    CmdHistory *m_cmdHistory;
    QString m_bookmarksFilename;
    QString m_caption;
    QString m_dbusObjectName;

    BookmarkInfoWidget *m_bkinfo;

    bool m_canPaste:1;
    bool m_readOnly:1;
    bool m_browser:1;
};

#endif
