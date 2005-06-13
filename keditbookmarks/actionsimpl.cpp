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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "actionsimpl.h"

#include "toplevel.h"
#include "commands.h"
#include "importers.h"
#include "favicons.h"
#include "testlink.h"
#include "listview.h"
#include "exporters.h"

#include <stdlib.h>

#include <qclipboard.h>
#include <qpopupmenu.h>
#include <qpainter.h>

#include <klocale.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <kdebug.h>
#include <kapplication.h>

#include <kaction.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <krun.h>

#include <kdatastream.h>
#include <ktempfile.h>
#include <kstandarddirs.h>

#include <kparts/part.h>
#include <kparts/componentfactory.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>

#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
#include <kbookmarkexporter.h>

ActionsImpl* ActionsImpl::s_self = 0;

// decoupled from resetActions in toplevel.cpp
// as resetActions simply uses the action groups
// specified in the ui.rc file
void KEBApp::createActions() {

    ActionsImpl *actn = ActionsImpl::self();

    // save and quit should probably not be in the toplevel???
    (void) KStdAction::quit(
        this, SLOT( close() ), actionCollection());
    KStdAction::keyBindings(guiFactory(), SLOT(configureShortcuts()), 
actionCollection());
    (void) KStdAction::configureToolbars(
        this, SLOT( slotConfigureToolbars() ), actionCollection());

    (void) KStdAction::save(
        actn, SLOT( slotSave() ), actionCollection());
    if (m_browser) {
        (void) KStdAction::open(
            actn, SLOT( slotLoad() ), actionCollection());
        (void) KStdAction::saveAs(
            actn, SLOT( slotSaveAs() ), actionCollection());
    }

    (void) KStdAction::cut(actn, SLOT( slotCut() ), actionCollection());
    (void) KStdAction::copy(actn, SLOT( slotCopy() ), actionCollection());
    (void) KStdAction::paste(actn, SLOT( slotPaste() ), actionCollection());
    (void) KStdAction::print(actn, SLOT( slotPrint() ), actionCollection());

    // settings menu
    (void) new KToggleAction(
        i18n("&Auto-Save on Program Close"), 0,
        this, SLOT( slotSaveOnClose() ), actionCollection(),
        "settings_saveonclose");

    /* (void) new KToggleAction(
            i18n("Split View (Very Experimental!)"), 0,
            this, SLOT( slotSplitView() ), actionCollection(),
            "settings_splitview"); */
    (void) new KToggleAction(
        i18n("&Show Netscape Bookmarks in Konqueror"), 0,
        actn, SLOT( slotShowNS() ), actionCollection(),
        "settings_showNS");

    // actions
    (void) new KAction(
        i18n("&Delete"), "editdelete", Key_Delete,
        actn, SLOT( slotDelete() ), actionCollection(), "delete");
    (void) new KAction(
        i18n("Rename"), "text", Key_F2,
        actn, SLOT( slotRename() ), actionCollection(), "rename");
    (void) new KAction(
        i18n("C&hange URL"), "text", Key_F3,
        actn, SLOT( slotChangeURL() ), actionCollection(), "changeurl");
    (void) new KAction(
        i18n("C&hange Comment"), "text", Key_F4,
        actn, SLOT( slotChangeComment() ), actionCollection(), "changecomment");
    (void) new KAction(
        i18n("Chan&ge Icon..."), "icons", 0,
        actn, SLOT( slotChangeIcon() ), actionCollection(), "changeicon");
    (void) new KAction(
        i18n("Update Favicon"), 0,
        actn, SLOT( slotUpdateFavIcon() ), actionCollection(), "updatefavicon");
    (void) new KAction(
        i18n("Recursive Sort"), 0,
        actn, SLOT( slotRecursiveSort() ), actionCollection(), "recursivesort");
    (void) new KAction(
        i18n("&New Folder..."), "folder_new", CTRL+Key_N,
        actn, SLOT( slotNewFolder() ), actionCollection(), "newfolder");
    (void) new KAction(
        i18n("&New Bookmark"), "www", 0,
        actn, SLOT( slotNewBookmark() ), actionCollection(), "newbookmark");
    (void) new KAction(
        i18n("&Insert Separator"), CTRL+Key_I,
        actn, SLOT( slotInsertSeparator() ), actionCollection(),
        "insertseparator");
    (void) new KAction(
        i18n("&Sort Alphabetically"), 0,
        actn, SLOT( slotSort() ), actionCollection(), "sort");
    (void) new KAction(
        i18n("Set as T&oolbar Folder"), "bookmark_toolbar", 0,
        actn, SLOT( slotSetAsToolbar() ), actionCollection(), "setastoolbar");
    (void) new KAction(
        i18n("Show in T&oolbar"), "bookmark_toolbar", 0,
        actn, SLOT( slotShowInToolbar() ), actionCollection(), "showintoolbar");
    (void) new KAction(
        i18n("&Expand All Folders"), 0,
        actn, SLOT( slotExpandAll() ), actionCollection(), "expandall");
    (void) new KAction(
        i18n("Collapse &All Folders"), 0,
        actn, SLOT( slotCollapseAll() ), actionCollection(), "collapseall" );
    (void) new KAction(
        i18n("&Open in Konqueror"), "fileopen", 0,
        actn, SLOT( slotOpenLink() ), actionCollection(), "openlink" );
    (void) new KAction(
        i18n("Check &Status"), "bookmark", 0,
        actn, SLOT( slotTestSelection() ), actionCollection(), "testlink" );

    (void) new KAction(
        i18n("Check Status: &All"), 0,
        actn, SLOT( slotTestAll() ), actionCollection(), "testall" );
    (void) new KAction(
        i18n("Update All &Favicons"), 0,
        actn, SLOT( slotUpdateAllFavIcons() ), actionCollection(),
        "updateallfavicons" );
    (void) new KAction(
        i18n("Cancel &Checks"), 0,
        actn, SLOT( slotCancelAllTests() ), actionCollection(), "canceltests" );
    (void) new KAction(
        i18n("Cancel &Favicon Updates"), 0,
        actn, SLOT( slotCancelFavIconUpdates() ), actionCollection(),
        "cancelfaviconupdates" );
    (void) new KAction(
        i18n("Import &Netscape Bookmarks..."), "netscape", 0,
        actn, SLOT( slotImport() ), actionCollection(), "importNS");
    (void) new KAction(
        i18n("Import &Opera Bookmarks..."), "opera", 0,
        actn, SLOT( slotImport() ), actionCollection(), "importOpera");
    (void) new KAction(
        i18n("Import All &Crash Sessions as Bookmarks..."), 0,
        actn, SLOT( slotImport() ), actionCollection(), "importCrashes");
    (void) new KAction(
        i18n("Import &Galeon Bookmarks..."), 0,
        actn, SLOT( slotImport() ), actionCollection(), "importGaleon");
    (void) new KAction(
        i18n("Import &KDE2 Bookmarks..."), 0,
        actn, SLOT( slotImport() ), actionCollection(), "importKDE2");
    (void) new KAction(
        i18n("Import &IE Bookmarks..."), 0,
        actn, SLOT( slotImport() ), actionCollection(), "importIE");
    (void) new KAction(
        i18n("Import &Mozilla Bookmarks..."), "mozilla", 0,
        actn, SLOT( slotImport() ), actionCollection(), "importMoz");
    (void) new KAction(
        i18n("Export to &Netscape Bookmarks"), "netscape", 0,
        actn, SLOT( slotExportNS() ), actionCollection(), "exportNS");
    (void) new KAction(
        i18n("Export to &Opera Bookmarks..."), "opera", 0,
        actn, SLOT( slotExportOpera() ), actionCollection(), "exportOpera");
    (void) new KAction(
        i18n("Export to &HTML Bookmarks..."), "html", 0,
        actn, SLOT( slotExportHTML() ), actionCollection(), "exportHTML");
    (void) new KAction(
        i18n("Export to &IE Bookmarks..."), 0,
        actn, SLOT( slotExportIE() ), actionCollection(), "exportIE");
    (void) new KAction(
        i18n("Export to &Mozilla Bookmarks..."), "mozilla", 0,
        actn, SLOT( slotExportMoz() ), actionCollection(), "exportMoz");
}

bool ActionsImpl::save() {
    if (!CurrentMgr::self()->managerSave())
        return false;
    CurrentMgr::self()->notifyManagers();
    KEBApp::self()->setModifiedFlag(false);
    KEBApp::self()->m_cmdHistory->notifyDocSaved();
    return true;
}

bool ActionsImpl::queryClose() {
    if (!KEBApp::self()->m_modified)
        return true;
    if (KEBApp::self()->m_saveOnClose)
        return save();

    switch (
            KMessageBox::warningYesNoCancel(
                KEBApp::self(), i18n("The bookmarks have been modified.\n"
                    "Save changes?"),
                QString::null, KStdGuiItem::save(), KStdGuiItem::discard())
           ) {
        case KMessageBox::Yes:
            return save();
        case KMessageBox::No:
            return true;
        default: // case KMessageBox::Cancel:
            return false;
    }
}

void ActionsImpl::slotLoad() {
    if (!queryClose())
        return;
    QString bookmarksFile
        = KFileDialog::getOpenFileName(QString::null, "*.xml", KEBApp::self());
    if (bookmarksFile.isNull())
        return;
    KEBApp::self()->m_caption = QString::null;
    KEBApp::self()->m_bookmarksFilename = bookmarksFile;
    KEBApp::self()->construct();
}

void ActionsImpl::slotSave() {
    (void)save();
}

void ActionsImpl::slotSaveAs() {
    QString saveFilename
        = KFileDialog::getSaveFileName(QString::null, "*.xml", KEBApp::self());
    if (!saveFilename.isEmpty())
        CurrentMgr::self()->saveAs(saveFilename);
}

void CurrentMgr::doExport(ExportType type, const QString & _path) {
    QString path(_path);
    // TODO - add a factory and make all this use the base class
    if (type == OperaExport) {
        if (path.isNull())
            path = KOperaBookmarkImporterImpl().findDefaultLocation(true);
        KOperaBookmarkExporterImpl exporter(mgr(), path);
        exporter.write(mgr()->root());
        return;

    } else if (type == HTMLExport) {
        if (path.isNull())
            path = KFileDialog::getSaveFileName(
                        QDir::homeDirPath(),
                        i18n("*.html|HTML Bookmark Listing") );
        HTMLExporter exporter;
        exporter.write(mgr()->root(), path);
        return;

    } else if (type == IEExport) {
        if (path.isNull())
            path = KIEBookmarkImporterImpl().findDefaultLocation(true);
        KIEBookmarkExporterImpl exporter(mgr(), path);
        exporter.write(mgr()->root());
        return;
    }

    bool moz = (type == MozillaExport);

    if (path.isNull())
        path = (moz) ? KNSBookmarkImporter::mozillaBookmarksFile(true)
            : KNSBookmarkImporter::netscapeBookmarksFile(true);

    if (!path.isEmpty()) {
        KNSBookmarkExporter exporter(mgr(), path);
        exporter.write(moz);
    }
}

void KEBApp::setActionsEnabled(SelcAbilities sa) {
    KActionCollection * coll = actionCollection();

    QStringList toEnable;

    if (sa.itemSelected && !sa.root) {
        toEnable << "edit_copy";
        if (!sa.urlIsEmpty && !sa.group && !sa.separator)
            toEnable << "openlink";
    }

    if (!m_readOnly) {
        if (sa.notEmpty)
            toEnable << "testall" << "updateallfavicons";

        if (sa.itemSelected) {
            if (!sa.root)
                toEnable << "delete" << "edit_cut";
            if (m_canPaste)
                toEnable << "edit_paste";
            if (!sa.separator)
                toEnable << "testlink" << "updatefavicon";
            toEnable << "showintoolbar";
        }

        if (sa.singleSelect && !sa.root && !sa.separator) {
            toEnable << "rename" << "changeicon" << "changecomment";
            if (!sa.group)
                toEnable << "changeurl";
        }

        if (!sa.multiSelect) {
            toEnable << "newfolder" << "newbookmark" << "insertseparator";
            if (sa.group)
                toEnable << "sort" << "recursivesort" << "setastoolbar";
        }
    }

    QString stbString = sa.tbShowState ? i18n("Hide in T&oolbar") : i18n("Show in T&oolbar");
    coll->action("showintoolbar")->setText(stbString);

    for ( QStringList::Iterator it = toEnable.begin();
            it != toEnable.end(); ++it )
    {
        coll->action((*it).ascii())->setEnabled(true);
        // kdDebug() << (*it) << endl;
    }
}

void KEBApp::setCancelFavIconUpdatesEnabled(bool enabled) {
    actionCollection()->action("cancelfaviconupdates")->setEnabled(enabled);
}

void KEBApp::setCancelTestsEnabled(bool enabled) {
    actionCollection()->action("canceltests")->setEnabled(enabled);
}

void ActionsImpl::slotCut() {
    slotCopy();
    KMacroCommand *mcmd
        = CmdGen::self()->deleteItems(i18n("Cut Items"),
                                      ListView::self()->selectedItems());
    CmdHistory::self()->didCommand(mcmd);
}

void ActionsImpl::slotCopy() {
    // this is not a command, because it can't be undone
    Q_ASSERT(ListView::self()->selectedItems()->count() != 0);
    QValueList<KBookmark> bookmarks
        = ListView::self()->itemsToBookmarks(ListView::self()->selectedItems());
    KBookmarkDrag* data = KBookmarkDrag::newDrag(bookmarks, 0 /* not this ! */);
    kapp->clipboard()->setData(data, QClipboard::Clipboard);
}

void ActionsImpl::slotPaste() {
    KMacroCommand *mcmd =
        CmdGen::self()->insertMimeSource(
                            i18n("Paste"),
                            kapp->clipboard()->data(QClipboard::Clipboard),
                            ListView::self()->userAddress());
    CmdHistory::self()->didCommand(mcmd);
}

/* -------------------------------------- */

void ActionsImpl::slotNewFolder() {
    bool ok;
    QString str = KInputDialog::getText( i18n( "Create New Bookmark Folder" ),
            i18n( "New folder:" ), QString::null, &ok );
    if (!ok)
        return;

    CreateCommand *cmd = new CreateCommand(
                                ListView::self()->userAddress(),
                                str, "bookmark_folder", /*open*/ true);
    CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotNewBookmark() {
    // TODO - make a setCurrentItem(Command *) which uses finaladdress interface
    CreateCommand * cmd = new CreateCommand(
                                ListView::self()->userAddress(),
                                QString::null, "www", KURL("http://"));
    CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotInsertSeparator() {
    CreateCommand * cmd = new CreateCommand(ListView::self()->userAddress());
    CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotImport() {
    // kdDebug() << "ActionsImpl::slotImport() where sender()->name() == "
    //           << sender()->name() << endl;
    ImportCommand* import
        = ImportCommand::performImport(sender()->name()+6, KEBApp::self());
    if (!import)
        return;
    // TODO - following line doesn't work, fixme
    ListView::self()->setInitialAddress(import->groupAddress());
    CmdHistory::self()->addCommand(import);
}

// TODO - this is getting ugly and repetitive. cleanup!

void ActionsImpl::slotExportOpera() {
    CurrentMgr::self()->doExport(CurrentMgr::OperaExport); }
void ActionsImpl::slotExportHTML() {
    CurrentMgr::self()->doExport(CurrentMgr::HTMLExport); }
void ActionsImpl::slotExportIE() {
    CurrentMgr::self()->doExport(CurrentMgr::IEExport); }
void ActionsImpl::slotExportNS() {
    CurrentMgr::self()->doExport(CurrentMgr::NetscapeExport); }
void ActionsImpl::slotExportMoz() {
    CurrentMgr::self()->doExport(CurrentMgr::MozillaExport); }

/* -------------------------------------- */

static QCString s_appId, s_objId;
static KParts::ReadOnlyPart *s_part;

void ActionsImpl::slotPrint() {
    s_part = KParts::ComponentFactory
                        ::createPartInstanceFromQuery<KParts::ReadOnlyPart>(
                                "text/html", QString::null);
    s_part->setProperty("pluginsEnabled", QVariant(false, 1));
    s_part->setProperty("javaScriptEnabled", QVariant(false, 1));
    s_part->setProperty("javaEnabled", QVariant(false, 1));

    // doc->openStream( "text/html", KURL() );
    // doc->writeStream( QCString( "<HTML><BODY>FOO</BODY></HTML>" ) );
    // doc->closeStream();

    HTMLExporter exporter;
    KTempFile tmpf(locateLocal("tmp", "print_bookmarks"), ".html");
    QTextStream *tstream = tmpf.textStream();
    tstream->setEncoding(QTextStream::Unicode);
    (*tstream) << exporter.toString(CurrentMgr::self()->mgr()->root());
    tmpf.close();

    s_appId = kapp->dcopClient()->appId();
    s_objId = s_part->property("dcopObjectId").toString().latin1();
    connect(s_part, SIGNAL(completed()), this, SLOT(slotDelayedPrint()));

    s_part->openURL(KURL( tmpf.name() ));
}

void ActionsImpl::slotDelayedPrint() {
    Q_ASSERT(s_part);
    DCOPRef(s_appId, s_objId).send("print", false);
    delete s_part;
    s_part = 0;
}

/* -------------------------------------- */

void ActionsImpl::slotShowNS() {
    bool shown = KEBApp::self()->nsShown();
    CurrentMgr::self()->mgr()->setShowNSBookmarks(shown);
    KEBApp::self()->setModifiedFlag(true);
    // TODO - need to force a save here
    CurrentMgr::self()->reloadConfig();
}

void ActionsImpl::slotCancelFavIconUpdates() {
    FavIconsItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotCancelAllTests() {
    TestLinkItrHolder::self()->cancelAllItrs();
}

void ActionsImpl::slotTestAll() {
    TestLinkItrHolder::self()->insertItr(
            new TestLinkItr(ListView::self()->allBookmarks()));
}

void ActionsImpl::slotUpdateAllFavIcons() {
    FavIconsItrHolder::self()->insertItr(
            new FavIconsItr(ListView::self()->allBookmarks()));
}

ActionsImpl::~ActionsImpl() {
    delete FavIconsItrHolder::self();
    delete TestLinkItrHolder::self();
}

/* -------------------------------------- */

void ActionsImpl::slotTestSelection() {
    TestLinkItrHolder::self()->insertItr(new TestLinkItr(ListView::self()->selectedBookmarksExpanded()));
}

void ActionsImpl::slotUpdateFavIcon() {
    FavIconsItrHolder::self()->insertItr(new FavIconsItr(ListView::self()->selectedBookmarksExpanded()));
}

/* -------------------------------------- */

class KBookmarkGroupList : private KBookmarkGroupTraverser {
public:
    KBookmarkGroupList(KBookmarkManager *);
    QValueList<KBookmark> getList(const KBookmarkGroup &);
private:
    virtual void visit(const KBookmark &) { ; }
    virtual void visitEnter(const KBookmarkGroup &);
    virtual void visitLeave(const KBookmarkGroup &) { ; }
private:
    KBookmarkManager *m_manager;
    QValueList<KBookmark> m_list;
};

KBookmarkGroupList::KBookmarkGroupList( KBookmarkManager *manager ) {
    m_manager = manager;
}

QValueList<KBookmark> KBookmarkGroupList::getList( const KBookmarkGroup &grp ) {
    traverse(grp);
    return m_list;
}

void KBookmarkGroupList::visitEnter(const KBookmarkGroup &grp) {
    m_list << grp;
}

void ActionsImpl::slotRecursiveSort() {
    KBookmark bk = ListView::self()->selectedItems()->first()->bookmark();
    Q_ASSERT(bk.isGroup());
    KMacroCommand *mcmd = new KMacroCommand(i18n("Recursive Sort"));
    KBookmarkGroupList lister(CurrentMgr::self()->mgr());
    QValueList<KBookmark> bookmarks = lister.getList(bk.toGroup());
    bookmarks << bk.toGroup();
    for (QValueListConstIterator<KBookmark> it = bookmarks.begin(); it != bookmarks.end(); ++it) {
        SortCommand *cmd = new SortCommand("", (*it).address());
        cmd->execute();
        mcmd->addCommand(cmd);
    }
    CmdHistory::self()->didCommand(mcmd);
}

void ActionsImpl::slotSort() {
    KBookmark bk = ListView::self()->selectedItems()->first()->bookmark();
    Q_ASSERT(bk.isGroup());
    SortCommand *cmd = new SortCommand(i18n("Sort Alphabetically"), bk.address());
    CmdHistory::self()->addCommand(cmd);
}

/* -------------------------------------- */

void ActionsImpl::slotDelete() {
    KMacroCommand *mcmd = CmdGen::self()->deleteItems(i18n("Delete Items"), ListView::self()->selectedItems());
    CmdHistory::self()->didCommand(mcmd);
}

void ActionsImpl::slotOpenLink() {
    QValueList<KBookmark> bks = ListView::self()->itemsToBookmarks(ListView::self()->selectedItems());
    QValueListIterator<KBookmark> it;
    for (it = bks.begin(); it != bks.end(); ++it) {
        if ((*it).isGroup() || (*it).isSeparator())
            continue;
        (void)new KRun((*it).url());
    }
}

/* -------------------------------------- */

void ActionsImpl::slotRename() {
    ListView::self()->rename(KEBListView::NameColumn);
}

void ActionsImpl::slotChangeURL() {
    ListView::self()->rename(KEBListView::UrlColumn);
}

void ActionsImpl::slotChangeComment() {
    ListView::self()->rename(KEBListView::CommentColumn);
}

void ActionsImpl::slotSetAsToolbar() {
    KBookmark bk = ListView::self()->selectedItems()->first()->bookmark();
    Q_ASSERT(bk.isGroup());
    KMacroCommand *mcmd = CmdGen::self()->setAsToolbar(bk);
    CmdHistory::self()->addCommand(mcmd);
}

void ActionsImpl::slotShowInToolbar() {
    KBookmark bk = ListView::self()->selectedItems()->first()->bookmark();
    bool shown = CmdGen::self()->shownInToolbar(bk);
    KMacroCommand *mcmd = CmdGen::self()->setShownInToolbar(bk, !shown);
    CmdHistory::self()->addCommand(mcmd);
}

void ActionsImpl::slotChangeIcon() {
    KBookmark bk = ListView::self()->selectedItems()->first()->bookmark();
    KIconDialog dlg(KEBApp::self());
    QString newIcon = dlg.selectIcon(KIcon::Small, KIcon::FileSystem);
    if (newIcon.isEmpty())
        return;
    EditCommand *cmd = new EditCommand(
                                bk.address(),
                                EditCommand::Edition("icon", newIcon),
                                i18n("Icon"));
    CmdHistory::self()->addCommand(cmd);
}

void ActionsImpl::slotExpandAll() {
    ListView::self()->setOpen(true);
    KEBApp::self()->setModifiedFlag(true);
}

void ActionsImpl::slotCollapseAll() {
    ListView::self()->setOpen(false);
    KEBApp::self()->setModifiedFlag(true);
}

#include "actionsimpl.moc"
