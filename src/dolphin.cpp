/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Stefan Monov <logixoul@gmail.com>               *
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "dolphin.h"

#include <assert.h>

#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kbookmarkmanager.h>
#include <kglobal.h>
#include <kpropertiesdialog.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kdeversion.h>
#include <kstatusbar.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <kconfig.h>
#include <kurl.h>
#include <kstdaccel.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kmenu.h>
#include <kio/renamedlg.h>
#include <kinputdialog.h>
#include <kshell.h>
#include <kdesktopfile.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <kmessagebox.h>
#include <kservice.h>
#include <kstandarddirs.h>
#include <krun.h>
#include <klocale.h>

#include <qclipboard.h>
#include <q3dragobject.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <QCloseEvent>
#include <QSplitter>

#include "urlnavigator.h"
#include "viewpropertiesdialog.h"
#include "viewproperties.h"
#include "dolphinsettings.h"
#include "dolphinsettingsdialog.h"
#include "dolphinstatusbar.h"
#include "undomanager.h"
#include "progressindicator.h"
#include "dolphinsettings.h"
#include "sidebar.h"
#include "sidebarsettings.h"
#include "generalsettings.h"

Dolphin& Dolphin::mainWin()
{
    static Dolphin* instance = 0;
    if (instance == 0) {
        instance = new Dolphin();
        instance->init();
    }
    return *instance;
}

Dolphin::~Dolphin()
{
}

void Dolphin::setActiveView(DolphinView* view)
{
    assert((view == m_view[PrimaryIdx]) || (view == m_view[SecondaryIdx]));
    if (m_activeView == view) {
        return;
    }

    m_activeView = view;

    updateHistory();
    updateEditActions();
    updateViewActions();
    updateGoActions();

    setCaption(m_activeView->url().fileName());

    emit activeViewChanged();
}

void Dolphin::dropURLs(const KUrl::List& urls,
                       const KUrl& destination)
{
    int selectedIndex = -1;

    /* KDE4-TODO
    const ButtonState keyboardState = KApplication::keyboardMouseState();
    const bool shiftPressed = (keyboardState & ShiftButton) > 0;
    const bool controlPressed = (keyboardState & ControlButton) > 0;



    if (shiftPressed && controlPressed) {
        // shortcut for 'Linke Here' is used
        selectedIndex = 2;
    }
    else if (controlPressed) {
        // shortcut for 'Copy Here' is used
        selectedIndex = 1;
    }
    else if (shiftPressed) {
        // shortcut for 'Move Here' is used
        selectedIndex = 0;
    }
    else*/ {
        // no shortcut is used, hence open a popup menu
        KMenu popup(this);

        popup.insertItem(SmallIcon("goto"), i18n("&Move Here") + "\t" /* KDE4-TODO: + KKey::modFlagLabel(KKey::SHIFT)*/, 0);
        popup.insertItem(SmallIcon("editcopy"), i18n( "&Copy Here" ) /* KDE4-TODO + "\t" + KKey::modFlagLabel(KKey::CTRL)*/, 1);
        popup.insertItem(i18n("&Link Here") /* KDE4-TODO + "\t" + KKey::modFlagLabel((KKey::ModFlag)(KKey::CTRL|KKey::SHIFT)) */, 2);
        popup.insertSeparator();
        popup.insertItem(SmallIcon("stop"), i18n("Cancel"), 3);
        popup.setAccel(i18n("Escape"), 3);

        /* KDE4-TODO: selectedIndex = popup.exec(QCursor::pos()); */
        popup.exec(QCursor::pos());
        selectedIndex = 0; // KD4-TODO: use QAction instead of switch below
    }

    if (selectedIndex < 0) {
        return;
    }

    switch (selectedIndex) {
        case 0: {
            // 'Move Here' has been selected
            updateViewProperties(urls);
            moveURLs(urls, destination);
            break;
        }

        case 1: {
            // 'Copy Here' has been selected
            updateViewProperties(urls);
            copyURLs(urls, destination);
            break;
        }

        case 2: {
            // 'Link Here' has been selected
            KIO::Job* job = KIO::link(urls, destination);
            addPendingUndoJob(job, DolphinCommand::Link, urls, destination);
            break;
        }

        default:
            // 'Cancel' has been selected
            break;
    }
}

void Dolphin::refreshViews()
{
    const bool split = DolphinSettings::instance().generalSettings()->splitView();
    const bool isPrimaryViewActive = (m_activeView == m_view[PrimaryIdx]);
    KUrl url;
    for (int i = PrimaryIdx; i <= SecondaryIdx; ++i) {
       if (m_view[i] != 0) {
            url = m_view[i]->url();

            // delete view instance...
            m_view[i]->close();
            m_view[i]->deleteLater();
            m_view[i] = 0;
        }

        if (split || (i == PrimaryIdx)) {
            // ... and recreate it
            ViewProperties props(url);
            m_view[i] = new DolphinView(m_splitter,
                                        url,
                                        props.viewMode(),
                                        props.isShowHiddenFilesEnabled());
            m_view[i]->show();
        }
    }

    m_activeView = isPrimaryViewActive ? m_view[PrimaryIdx] : m_view[SecondaryIdx];
    assert(m_activeView != 0);

    updateViewActions();
    emit activeViewChanged();
}

void Dolphin::slotHistoryChanged()
{
    updateHistory();
}

void Dolphin::slotURLChanged(const KUrl& url)
{
    updateEditActions();
    updateGoActions();
    setCaption(url.fileName());
}

void Dolphin::slotURLChangeRequest(const KUrl& url)
{
	clearStatusBar();
	m_activeView->setURL(url);
}

void Dolphin::slotViewModeChanged()
{
    updateViewActions();
}

void Dolphin::slotShowHiddenFilesChanged()
{
    KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    showHiddenFilesAction->setChecked(m_activeView->isShowHiddenFilesEnabled());
}

void Dolphin::slotShowFilterBarChanged()
{
    KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    showFilterBarAction->setChecked(m_activeView->isFilterBarVisible());
}

void Dolphin::slotSortingChanged(DolphinView::Sorting sorting)
{
    KAction* action = 0;
    switch (sorting) {
        case DolphinView::SortByName:
            action = actionCollection()->action("by_name");
            break;
        case DolphinView::SortBySize:
            action = actionCollection()->action("by_size");
            break;
        case DolphinView::SortByDate:
            action = actionCollection()->action("by_date");
            break;
        default:
            break;
    }

    if (action != 0) {
        KToggleAction* toggleAction = static_cast<KToggleAction*>(action);
        toggleAction->setChecked(true);
    }
}

void Dolphin::slotSortOrderChanged(Qt::SortOrder order)
{
    KToggleAction* descending = static_cast<KToggleAction*>(actionCollection()->action("descending"));
    const bool sortDescending = (order == Qt::Descending);
    descending->setChecked(sortDescending);
}

void Dolphin::slotSelectionChanged()
{
    updateEditActions();

    assert(m_view[PrimaryIdx] != 0);
    int selectedURLsCount = m_view[PrimaryIdx]->selectedURLs().count();
    if (m_view[SecondaryIdx] != 0) {
        selectedURLsCount += m_view[SecondaryIdx]->selectedURLs().count();
    }

    KAction* compareFilesAction = actionCollection()->action("compare_files");
    compareFilesAction->setEnabled(selectedURLsCount == 2);

    m_activeView->updateStatusBar();

    emit selectionChanged();
}

void Dolphin::closeEvent(QCloseEvent* event)
{
    // KDE4-TODO
    //KConfig* config = KGlobal::config();
    //config->setGroup("General");
    //config->writeEntry("First Run", false);

    DolphinSettings& settings = DolphinSettings::instance();
    GeneralSettings* generalSettings = settings.generalSettings();
    generalSettings->setFirstRun(false);

    SidebarSettings* sidebarSettings = settings.sidebarSettings();
    const bool isSidebarVisible = (m_sidebar != 0);
    sidebarSettings->setVisible(isSidebarVisible);
    if (isSidebarVisible) {
        sidebarSettings->setWidth(m_sidebar->width());
    }

    settings.save();

    KMainWindow::closeEvent(event);
}

void Dolphin::saveProperties(KConfig* config)
{
    config->setGroup("Primary view");
    config->writeEntry("URL", m_view[PrimaryIdx]->url().url());
    config->writeEntry("Editable URL", m_view[PrimaryIdx]->isURLEditable());
    if (m_view[SecondaryIdx] != 0) {
        config->setGroup("Secondary view");
        config->writeEntry("URL", m_view[SecondaryIdx]->url().url());
        config->writeEntry("Editable URL", m_view[SecondaryIdx]->isURLEditable());
    }
}

void Dolphin::readProperties(KConfig* config)
{
    config->setGroup("Primary view");
    m_view[PrimaryIdx]->setURL(config->readEntry("URL"));
    m_view[PrimaryIdx]->setURLEditable(config->readBoolEntry("Editable URL"));
    if (config->hasGroup("Secondary view")) {
        config->setGroup("Secondary view");
        if (m_view[SecondaryIdx] == 0) {
            toggleSplitView();
        }
        m_view[SecondaryIdx]->setURL(config->readEntry("URL"));
        m_view[SecondaryIdx]->setURLEditable(config->readBoolEntry("Editable URL"));
    }
    else if (m_view[SecondaryIdx] != 0) {
        toggleSplitView();
    }
}

void Dolphin::createFolder()
{
    // Parts of the following code have been taken
    // from the class KonqPopupMenu located in
    // libqonq/konq_popupmenu.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>,
    // Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>)

    clearStatusBar();

    DolphinStatusBar* statusBar = m_activeView->statusBar();
    const KUrl baseURL(m_activeView->url());

    QString name(i18n("New Folder"));
    baseURL.path(KUrl::AddTrailingSlash);


    if (baseURL.isLocalFile() && QFileInfo(baseURL.path(KUrl::AddTrailingSlash) + name).exists()) {
        name = KIO::RenameDlg::suggestName(baseURL, i18n("New Folder"));
    }

    bool ok = false;
    name = KInputDialog::getText(i18n("New Folder"),
                                 i18n("Enter folder name:" ),
                                 name,
                                 &ok,
                                 this);

    if (!ok) {
        // the user has pressed 'Cancel'
        return;
    }

    assert(!name.isEmpty());

    KUrl url;
    if ((name[0] == '/') || (name[0] == '~')) {
        url.setPath(KShell::tildeExpand(name));
    }
    else {
        name = KIO::encodeFileName(name);
        url = baseURL;
        url.addPath(name);
    }
    ok = KIO::NetAccess::mkdir(url, this);

    // TODO: provide message type hint
    if (ok) {
        statusBar->setMessage(i18n("Created folder %1.").arg(url.path()),
                              DolphinStatusBar::OperationCompleted);

        DolphinCommand command(DolphinCommand::CreateFolder, KUrl::List(), url);
        UndoManager::instance().addCommand(command);
    }
    else {
        // Creating of the folder has been failed. Check whether the creating
        // has been failed because a folder with the same name exists...
        if (KIO::NetAccess::exists(url, true, this)) {
            statusBar->setMessage(i18n("A folder named %1 already exists.").arg(url.path()),
                                  DolphinStatusBar::Error);
        }
        else {
            statusBar->setMessage(i18n("Creating of folder %1 failed.").arg(url.path()),
                                  DolphinStatusBar::Error);
        }

    }
}

void Dolphin::createFile()
{
    // Parts of the following code have been taken
    // from the class KonqPopupMenu located in
    // libqonq/konq_popupmenu.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>,
    // Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>)

    clearStatusBar();

    // TODO: const Entry& entry = m_createFileTemplates[QString(sender->name())];
    // should be enough. Anyway: the implemantation of [] does a linear search internally too.
    KSortableList<CreateFileEntry, QString>::ConstIterator it = m_createFileTemplates.begin();
    KSortableList<CreateFileEntry, QString>::ConstIterator end = m_createFileTemplates.end();

    const QString senderName(sender()->name());
    bool found = false;
    CreateFileEntry entry;
    while (!found && (it != end)) {
        if ((*it).index() == senderName) {
            entry = (*it).value();
            found = true;
        }
        else {
            ++it;
        }
    }

    DolphinStatusBar* statusBar = m_activeView->statusBar();
    if (!found || !QFile::exists(entry.templatePath)) {
        statusBar->setMessage(i18n("Could not create file."), DolphinStatusBar::Error);
       return;
    }

    // Get the source path of the template which should be copied.
    // The source path is part of the URL entry of the desktop file.
    const int pos = entry.templatePath.findRev('/');
    QString sourcePath(entry.templatePath.left(pos + 1));
    sourcePath += KDesktopFile(entry.templatePath, true).readPathEntry("URL");

    QString name(i18n(entry.name.ascii()));
    // Most entry names end with "..." (e. g. "HTML File..."), which is ok for
    // menus but no good choice for a new file name -> remove the dots...
    name.replace("...", QString::null);

    // add the file extension to the name
    name.append(sourcePath.right(sourcePath.length() - sourcePath.findRev('.')));

    // Check whether a file with the current name already exists. If yes suggest automatically
    // a unique file name (e. g. "HTML File" will be replaced by "HTML File_1").
    const KUrl viewURL(m_activeView->url());
    const bool fileExists = viewURL.isLocalFile() &&
                            QFileInfo(viewURL.path(KUrl::AddTrailingSlash) + KIO::encodeFileName(name)).exists();
    if (fileExists) {
        name = KIO::RenameDlg::suggestName(viewURL, name);
    }

    // let the user change the suggested file name
    bool ok = false;
    name = KInputDialog::getText(entry.name,
                                 entry.comment,
                                 name,
                                 &ok,
                                 this);
    if (!ok) {
        // the user has pressed 'Cancel'
        return;
    }

    // before copying the template to the destination path check whether a file
    // with the given name already exists
    const QString destPath(viewURL.pathOrUrl() + "/" + KIO::encodeFileName(name));
    const KUrl destURL(destPath);
    if (KIO::NetAccess::exists(destURL, false, this)) {
        statusBar->setMessage(i18n("A file named %1 already exists.").arg(name),
                              DolphinStatusBar::Error);
        return;
    }

    // copy the template to the destination path
    const KUrl sourceURL(sourcePath);
    KIO::CopyJob* job = KIO::copyAs(sourceURL, destURL);
    job->setDefaultPermissions(true);
    if (KIO::NetAccess::synchronousRun(job, this)) {
        statusBar->setMessage(i18n("Created file %1.").arg(name),
                              DolphinStatusBar::OperationCompleted);

        KUrl::List list;
        list.append(sourceURL);
        DolphinCommand command(DolphinCommand::CreateFile, list, destURL);
        UndoManager::instance().addCommand(command);

    }
    else {
        statusBar->setMessage(i18n("Creating of file %1 failed.").arg(name),
                              DolphinStatusBar::Error);
    }
}

void Dolphin::rename()
{
    clearStatusBar();
    m_activeView->renameSelectedItems();
}

void Dolphin::moveToTrash()
{
    clearStatusBar();
    KUrl::List selectedURLs = m_activeView->selectedURLs();
    KIO::Job* job = KIO::trash(selectedURLs);
    addPendingUndoJob(job, DolphinCommand::Trash, selectedURLs, m_activeView->url());
}

void Dolphin::deleteItems()
{
    clearStatusBar();

    KUrl::List list = m_activeView->selectedURLs();
    const uint itemCount = list.count();
    assert(itemCount >= 1);

    QString text;
    if (itemCount > 1) {
        text = i18n("Do you really want to delete the %1 selected items?").arg(itemCount);
    }
    else {
        const KUrl& url = list.first();
        text = i18n("Do you really want to delete '%1'?").arg(url.fileName());
    }

    const bool del = KMessageBox::warningContinueCancel(this,
                                                        text,
                                                        QString::null,
                                                        KGuiItem(i18n("Delete"), SmallIcon("editdelete"))
                                                       ) == KMessageBox::Continue;
    if (del) {
        KIO::Job* job = KIO::del(list);
        connect(job, SIGNAL(result(KIO::Job*)),
                this, SLOT(slotHandleJobError(KIO::Job*)));
        connect(job, SIGNAL(result(KIO::Job*)),
                this, SLOT(slotDeleteFileFinished(KIO::Job*)));
    }
}

void Dolphin::properties()
{
    const KFileItemList* sourceList = m_activeView->selectedItems();
    if (sourceList == 0) {
        return;
    }

    KFileItemList list;
    KFileItemList::const_iterator it = sourceList->begin();
    const KFileItemList::const_iterator end = sourceList->end();
    KFileItem* item = 0;
    while (it != end) {
        list.append(item);
        ++it;
    }

    new KPropertiesDialog(list, this);
}

void Dolphin::quit()
{
    close();
}

void Dolphin::slotHandleJobError(KIO::Job* job)
{
    if (job->error() != 0) {
        m_activeView->statusBar()->setMessage(job->errorString(),
                                              DolphinStatusBar::Error);
    }
}

void Dolphin::slotDeleteFileFinished(KIO::Job* job)
{
    if (job->error() == 0) {
        m_activeView->statusBar()->setMessage(i18n("Delete operation completed."),
                                               DolphinStatusBar::OperationCompleted);

        // TODO: In opposite to the 'Move to Trash' operation in the class KFileIconView
        // no rearranging of the item position is done when a file has been deleted.
        // This is bypassed by reloading the view, but it might be worth to investigate
        // deeper for the root of this issue.
        m_activeView->reload();
    }
}

void Dolphin::slotUndoAvailable(bool available)
{
    KAction* undoAction = actionCollection()->action(KStdAction::stdName(KStdAction::Undo));
    if (undoAction != 0) {
        undoAction->setEnabled(available);
    }
}

void Dolphin::slotUndoTextChanged(const QString& text)
{
    KAction* undoAction = actionCollection()->action(KStdAction::stdName(KStdAction::Undo));
    if (undoAction != 0) {
        undoAction->setText(text);
    }
}

void Dolphin::slotRedoAvailable(bool available)
{
    KAction* redoAction = actionCollection()->action(KStdAction::stdName(KStdAction::Redo));
    if (redoAction != 0) {
        redoAction->setEnabled(available);
    }
}

void Dolphin::slotRedoTextChanged(const QString& text)
{
    KAction* redoAction = actionCollection()->action(KStdAction::stdName(KStdAction::Redo));
    if (redoAction != 0) {
        redoAction->setText(text);
    }
}

void Dolphin::cut()
{
    m_clipboardContainsCutData = true;
    /* KDE4-TODO: Q3DragObject* data = new KURLDrag(m_activeView->selectedURLs(),
                                       widget());
    QApplication::clipboard()->setData(data);*/
}

void Dolphin::copy()
{
    m_clipboardContainsCutData = false;
    /* KDE4-TODO:
    Q3DragObject* data = new KUrlDrag(m_activeView->selectedURLs(),
                                     widget());
    QApplication::clipboard()->setData(data);*/
}

void Dolphin::paste()
{
    /* KDE4-TODO:
    QClipboard* clipboard = QApplication::clipboard();
    QMimeSource* data = clipboard->data();
    if (!KUrlDrag::canDecode(data)) {
        return;
    }

    clearStatusBar();

    KUrl::List sourceURLs;
    KUrlDrag::decode(data, sourceURLs);

    // per default the pasting is done into the current URL of the view
    KUrl destURL(m_activeView->url());

    // check whether the pasting should be done into a selected directory
    KUrl::List selectedURLs = m_activeView->selectedURLs();
    if (selectedURLs.count() == 1) {
        const KFileItem fileItem(S_IFDIR,
                                 KFileItem::Unknown,
                                 selectedURLs.first(),
                                 true);
        if (fileItem.isDir()) {
            // only one item is selected which is a directory, hence paste
            // into this directory
            destURL = selectedURLs.first();
        }
    }


    updateViewProperties(sourceURLs);
    if (m_clipboardContainsCutData) {
        moveURLs(sourceURLs, destURL);
        m_clipboardContainsCutData = false;
        clipboard->clear();
    }
    else {
        copyURLs(sourceURLs, destURL);
    }*/
}

void Dolphin::updatePasteAction()
{
    KAction* pasteAction = actionCollection()->action(KStdAction::stdName(KStdAction::Paste));
    if (pasteAction == 0) {
        return;
    }

    QString text(i18n("Paste"));
    QClipboard* clipboard = QApplication::clipboard();
    QMimeSource* data = clipboard->data();
    /* KDE4-TODO:
    if (KUrlDrag::canDecode(data)) {
        pasteAction->setEnabled(true);

        KUrl::List urls;
        KUrlDrag::decode(data, urls);
        const int count = urls.count();
        if (count == 1) {
            pasteAction->setText(i18n("Paste 1 File"));
        }
        else {
            pasteAction->setText(i18n("Paste %1 Files").arg(count));
        }
    }
    else {*/
        pasteAction->setEnabled(false);
        pasteAction->setText(i18n("Paste"));
    //}

    if (pasteAction->isEnabled()) {
        KUrl::List urls = m_activeView->selectedURLs();
        const uint count = urls.count();
        if (count > 1) {
            // pasting should not be allowed when more than one file
            // is selected
            pasteAction->setEnabled(false);
        }
        else if (count == 1) {
            // Only one file is selected. Pasting is only allowed if this
            // file is a directory.
            const KFileItem fileItem(S_IFDIR,
                                     KFileItem::Unknown,
                                     urls.first(),
                                     true);
            pasteAction->setEnabled(fileItem.isDir());
        }
    }
}

void Dolphin::selectAll()
{
    clearStatusBar();
    m_activeView->selectAll();
}

void Dolphin::invertSelection()
{
    clearStatusBar();
    m_activeView->invertSelection();
}
void Dolphin::setIconsView()
{
    m_activeView->setMode(DolphinView::IconsView);
}

void Dolphin::setDetailsView()
{
    m_activeView->setMode(DolphinView::DetailsView);
}

void Dolphin::setPreviewsView()
{
    m_activeView->setMode(DolphinView::PreviewsView);
}

void Dolphin::sortByName()
{
    m_activeView->setSorting(DolphinView::SortByName);
}

void Dolphin::sortBySize()
{
    m_activeView->setSorting(DolphinView::SortBySize);
}

void Dolphin::sortByDate()
{
    m_activeView->setSorting(DolphinView::SortByDate);
}

void Dolphin::toggleSortOrder()
{
    const Qt::SortOrder order = (m_activeView->sortOrder() == Qt::Ascending) ?
                                Qt::Descending :
                                Qt::Ascending;
    m_activeView->setSortOrder(order);
}

void Dolphin::toggleSplitView()
{
    if (m_view[SecondaryIdx] == 0) {
        // create a secondary view
        m_view[SecondaryIdx] = new DolphinView(m_splitter,
                                               m_view[PrimaryIdx]->url(),
                                               m_view[PrimaryIdx]->mode(),
                                               m_view[PrimaryIdx]->isShowHiddenFilesEnabled());
        m_view[SecondaryIdx]->show();
    }
    else {
        // remove secondary view
        if (m_activeView == m_view[PrimaryIdx]) {
            m_view[SecondaryIdx]->close();
            m_view[SecondaryIdx]->deleteLater();
            m_view[SecondaryIdx] = 0;
            setActiveView(m_view[PrimaryIdx]);
        }
        else {
            // The secondary view is active, hence from the users point of view
            // the content of the secondary view should be moved to the primary view.
            // From an implementation point of view it is more efficient to close
            // the primary view and exchange the internal pointers afterwards.
            m_view[PrimaryIdx]->close();
            m_view[PrimaryIdx]->deleteLater();
            m_view[PrimaryIdx] = m_view[SecondaryIdx];
            m_view[SecondaryIdx] = 0;
            setActiveView(m_view[PrimaryIdx]);
        }
    }
}

void Dolphin::reloadView()
{
    clearStatusBar();
    m_activeView->reload();
}

void Dolphin::stopLoading()
{
}

void Dolphin::showHiddenFiles()
{
    clearStatusBar();

    const KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    const bool show = showHiddenFilesAction->isChecked();
    m_activeView->setShowHiddenFilesEnabled(show);
}

void Dolphin::showFilterBar()
{
    const KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    const bool show = showFilterBarAction->isChecked();
    m_activeView->slotShowFilterBar(show);
}

void Dolphin::zoomIn()
{
    m_activeView->zoomIn();
    updateViewActions();
}

void Dolphin::zoomOut()
{
    m_activeView->zoomOut();
    updateViewActions();
}

void Dolphin::toggleEditLocation()
{
    clearStatusBar();

    KToggleAction* action = static_cast<KToggleAction*>(actionCollection()->action("editable_location"));

    bool editOrBrowse = action->isChecked();
//    action->setChecked(action->setChecked);
    m_activeView->setURLEditable(editOrBrowse);
}

void Dolphin::editLocation()
{
    KToggleAction* action = static_cast<KToggleAction*>(actionCollection()->action("editable_location"));
    action->setChecked(true);
    m_activeView->setURLEditable(true);
}

void Dolphin::adjustViewProperties()
{
    clearStatusBar();
    ViewPropertiesDialog dlg(m_activeView);
    dlg.exec();
}

void Dolphin::goBack()
{
    clearStatusBar();
    m_activeView->goBack();
}

void Dolphin::goForward()
{
    clearStatusBar();
    m_activeView->goForward();
}

void Dolphin::goUp()
{
    clearStatusBar();
    m_activeView->goUp();
}

void Dolphin::goHome()
{
    clearStatusBar();
    m_activeView->goHome();
}

void Dolphin::openTerminal()
{
    QString command("konsole --workdir \"");
    command.append(m_activeView->url().path());
    command.append('\"');

    KRun::runCommand(command, "Konsole", "konsole");
}

void Dolphin::findFile()
{
    KRun::run("kfind", m_activeView->url());
}

void Dolphin::compareFiles()
{
    // The method is only invoked if exactly 2 files have
    // been selected. The selected files may be:
    // - both in the primary view
    // - both in the secondary view
    // - one in the primary view and the other in the secondary
    //   view
    assert(m_view[PrimaryIdx] != 0);

    KUrl urlA;
    KUrl urlB;
    KUrl::List urls = m_view[PrimaryIdx]->selectedURLs();

    switch (urls.count()) {
        case 0: {
            assert(m_view[SecondaryIdx] != 0);
            urls = m_view[SecondaryIdx]->selectedURLs();
            assert(urls.count() == 2);
            urlA = urls[0];
            urlB = urls[1];
            break;
        }

        case 1: {
            urlA = urls[0];
            assert(m_view[SecondaryIdx] != 0);
            urls = m_view[SecondaryIdx]->selectedURLs();
            assert(urls.count() == 1);
            urlB = urls[0];
            break;
        }

        case 2: {
            urlA = urls[0];
            urlB = urls[1];
            break;
        }

        default: {
            // may not happen: compareFiles may only get invoked if 2
            // files are selected
            assert(false);
        }
    }

    QString command("kompare -c \"");
    command.append(urlA.pathOrUrl());
    command.append("\" \"");
    command.append(urlB.pathOrUrl());
    command.append('\"');
    KRun::runCommand(command, "Kompare", "kompare");

}

void Dolphin::editSettings()
{
    // TODO: make a static method for opening the settings dialog
    DolphinSettingsDialog dlg;
    dlg.exec();
}

void Dolphin::addUndoOperation(KIO::Job* job)
{
    if (job->error() != 0) {
        slotHandleJobError(job);
    }
    else {
        const int id = job->progressId();

        // set iterator to the executed command with the current id...
        Q3ValueList<UndoInfo>::Iterator it = m_pendingUndoJobs.begin();
        const Q3ValueList<UndoInfo>::Iterator end = m_pendingUndoJobs.end();
        bool found = false;
        while (!found && (it != end)) {
            if ((*it).id == id) {
                found = true;
            }
            else {
                ++it;
            }
        }

        if (found) {
            DolphinCommand command = (*it).command;
            if (command.type() == DolphinCommand::Trash) {
                // To be able to perform an undo for the 'Move to Trash' operation
                // all source URLs must be updated with the trash URL. E. g. when moving
                // a file "test.txt" and a second file "test.txt" to the trash,
                // then the filenames in the trash are "0-test.txt" and "1-test.txt".
                QMap<QString, QString> metaData = job->metaData();
                KUrl::List newSourceURLs;

                KUrl::List sourceURLs = command.source();
                KUrl::List::Iterator sourceIt = sourceURLs.begin();
                const KUrl::List::Iterator sourceEnd = sourceURLs.end();

                while (sourceIt != sourceEnd) {
                    QMap<QString, QString>::ConstIterator metaIt = metaData.find("trashURL-" + (*sourceIt).path());
                    if (metaIt != metaData.end()) {
                        newSourceURLs.append(KUrl(metaIt.data()));
                    }
                    ++sourceIt;
                }
                command.setSource(newSourceURLs);
            }

            UndoManager::instance().addCommand(command);
            m_pendingUndoJobs.erase(it);

            DolphinStatusBar* statusBar = m_activeView->statusBar();
            switch (command.type()) {
                case DolphinCommand::Copy:
                    statusBar->setMessage(i18n("Copy operation completed."),
                                          DolphinStatusBar::OperationCompleted);
                    break;
                case DolphinCommand::Move:
                    statusBar->setMessage(i18n("Move operation completed."),
                                          DolphinStatusBar::OperationCompleted);
                    break;
                case DolphinCommand::Trash:
                    statusBar->setMessage(i18n("Move to trash operation completed."),
                                          DolphinStatusBar::OperationCompleted);
                    break;
                default:
                    break;
            }
        }
    }
}

void Dolphin::toggleSidebar()
{
    if (m_sidebar == 0) {
        openSidebar();
    }
    else {
        closeSidebar();
    }

    KToggleAction* sidebarAction = static_cast<KToggleAction*>(actionCollection()->action("sidebar"));
    sidebarAction->setChecked(m_sidebar != 0);
}

void Dolphin::closeSidebar()
{
    if (m_sidebar == 0) {
        // the sidebar has already been closed
        return;
    }

    // store width of sidebar and remember that the sidebar has been closed
    SidebarSettings* settings = DolphinSettings::instance().sidebarSettings();
    settings->setVisible(false);
    settings->setWidth(m_sidebar->width());

    m_sidebar->deleteLater();
    m_sidebar = 0;
}

Dolphin::Dolphin() :
    KMainWindow(0, "Dolphin"),
    m_splitter(0),
    m_sidebar(0),
    m_activeView(0),
    m_clipboardContainsCutData(false)
{
    m_view[PrimaryIdx] = 0;
    m_view[SecondaryIdx] = 0;

    m_fileGroupActions.setAutoDelete(true);

    // TODO: the following members are not used yet. See documentation
    // of Dolphin::linkGroupActions() and Dolphin::linkToDeviceActions()
    // in the header file for details.
    //m_linkGroupActions.setAutoDelete(true);
    //m_linkToDeviceActions.setAutoDelete(true);
}

void Dolphin::init()
{
    // Check whether Dolphin runs the first time. If yes then
    // a proper default window size is given at the end of Dolphin::init().
    GeneralSettings* generalSettings = DolphinSettings::instance().generalSettings();
    const bool firstRun = generalSettings->firstRun();

    setAcceptDrops(true);

    m_splitter = new QSplitter(this);

    DolphinSettings& settings = DolphinSettings::instance();

    KBookmarkManager* manager = settings.bookmarkManager();
    assert(manager != 0);
    KBookmarkGroup root = manager->root();
    if (root.first().isNull()) {
        root.addBookmark(manager, i18n("Home"), settings.generalSettings()->homeURL(), "folder_home");
        root.addBookmark(manager, i18n("Storage Media"), KUrl("media:/"), "blockdevice");
        root.addBookmark(manager, i18n("Network"), KUrl("remote:/"), "network_local");
        root.addBookmark(manager, i18n("Root"), KUrl("/"), "folder_red");
        root.addBookmark(manager, i18n("Trash"), KUrl("trash:/"), "trashcan_full");
    }

    setupActions();
    setupGUI(Keys|Save|Create|ToolBar);

    const KUrl& homeURL = root.first().url();
    setCaption(homeURL.fileName());
    ViewProperties props(homeURL);
    m_view[PrimaryIdx] = new DolphinView(m_splitter,
                                         homeURL,
                                         props.viewMode(),
                                         props.isShowHiddenFilesEnabled());

    m_activeView = m_view[PrimaryIdx];

    setCentralWidget(m_splitter);

    // open sidebar
    SidebarSettings* sidebarSettings = settings.sidebarSettings();
    assert(sidebarSettings != 0);
    if (sidebarSettings->visible()) {
        openSidebar();
    }

    createGUI();

    stateChanged("new_file");
    setAutoSaveSettings();

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));
    updatePasteAction();
    updateGoActions();

    setupCreateNewMenuActions();

    loadSettings();

    if (firstRun) {
        // assure a proper default size if Dolphin runs the first time
        resize(640, 480);
    }
}

void Dolphin::loadSettings()
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    KToggleAction* splitAction = static_cast<KToggleAction*>(actionCollection()->action("split_view"));
    if (settings->splitView()) {
        splitAction->setChecked(true);
        toggleSplitView();
    }

    updateViewActions();
}

void Dolphin::setupActions()
{
    // setup 'File' menu
    KAction* createFolder = new KAction(i18n("Folder..."), actionCollection(), "create_folder");
    createFolder->setIcon(KIcon("folder"));
    createFolder->setShortcut(Qt::Key_N);
    connect(createFolder, SIGNAL(triggered()), this, SLOT(createFolder()));

    KAction* rename = new KAction(i18n("Rename"), actionCollection(), "rename");
    rename->setShortcut(Qt::Key_F2);
    connect(rename, SIGNAL(triggered()), this, SLOT(rename()));

    KAction* moveToTrash = new KAction(i18n("Move to Trash"), actionCollection(), "move_to_trash");
    moveToTrash->setIcon(KIcon("edittrash"));
    moveToTrash->setShortcut(QKeySequence::Delete);
    connect(moveToTrash, SIGNAL(triggered()), this, SLOT(moveToTrash()));

    KAction* deleteAction = new KAction(i18n("Delete"), actionCollection(), "delete");
    deleteAction->setShortcut(Qt::ALT | Qt::Key_Delete);
    deleteAction->setIcon(KIcon("editdelete"));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteItems()));

    KAction* properties = new KAction(i18n("Propert&ies"), actionCollection(), "properties");
    properties->setShortcut(Qt::Key_Alt | Qt::Key_Return);
    connect(properties, SIGNAL(triggered()), this, SLOT(properties()));

    KStdAction::quit(this, SLOT(quit()), actionCollection());

    // setup 'Edit' menu
    UndoManager& undoManager = UndoManager::instance();
    KStdAction::undo(&undoManager,
                     SLOT(undo()),
                     actionCollection());
    connect(&undoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));
    connect(&undoManager, SIGNAL(undoTextChanged(const QString&)),
            this, SLOT(slotUndoTextChanged(const QString&)));

    KStdAction::redo(&undoManager,
                     SLOT(redo()),
                     actionCollection());
    connect(&undoManager, SIGNAL(redoAvailable(bool)),
            this, SLOT(slotRedoAvailable(bool)));
    connect(&undoManager, SIGNAL(redoTextChanged(const QString&)),
            this, SLOT(slotRedoTextChanged(const QString&)));

    KStdAction::cut(this, SLOT(cut()), actionCollection());
    KStdAction::copy(this, SLOT(copy()), actionCollection());
    KStdAction::paste(this, SLOT(paste()), actionCollection());

    KAction* selectAll = new KAction(i18n("Select All"), actionCollection(), "select_all");
    selectAll->setShortcut(Qt::CTRL + Qt::Key_A);
    connect(selectAll, SIGNAL(triggered()), this, SLOT(selectAll()));

    KAction* invertSelection = new KAction(i18n("Invert Selection"), actionCollection(), "invert_selection");
    invertSelection->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, SIGNAL(triggered()), this, SLOT(invertSelection()));

    // setup 'View' menu
    KStdAction::zoomIn(this,
                       SLOT(zoomIn()),
                       actionCollection());

    KStdAction::zoomOut(this,
                        SLOT(zoomOut()),
                        actionCollection());

    KToggleAction* iconsView = new KToggleAction(i18n("Icons"), actionCollection(), "icons");
    iconsView->setShortcut(Qt::CTRL | Qt::Key_1);
    iconsView->setIcon(KIcon("view_icon"));
    connect(iconsView, SIGNAL(triggered()), this, SLOT(setIconsView()));

    KToggleAction* detailsView = new KToggleAction(i18n("Details"), actionCollection(), "details");
    detailsView->setShortcut(Qt::CTRL | Qt::Key_2);
    detailsView->setIcon(KIcon("view_text"));
    connect(detailsView, SIGNAL(triggered()), this, SLOT(setIconsView()));

    KToggleAction* previewsView = new KToggleAction(i18n("Previews"), actionCollection(), "previews");
    previewsView->setShortcut(Qt::CTRL | Qt::Key_3);
    previewsView->setIcon(KIcon("gvdirpart"));
    connect(previewsView, SIGNAL(triggered()), this, SLOT(setPreviewsView()));

    QActionGroup* viewModeGroup = new QActionGroup(this);
    viewModeGroup->addAction(iconsView);
    viewModeGroup->addAction(detailsView);
    viewModeGroup->addAction(previewsView);

    KToggleAction* sortByName = new KToggleAction(i18n("By Name"), actionCollection(), "by_name");
    connect(sortByName, SIGNAL(triggered()), this, SLOT(sortByName()));

    KToggleAction* sortBySize = new KToggleAction(i18n("By Size"), actionCollection(), "by_size");
    connect(sortBySize, SIGNAL(triggered()), this, SLOT(sortBySize()));

    KToggleAction* sortByDate = new KToggleAction(i18n("By Date"), actionCollection(), "by_date");
    connect(sortByDate, SIGNAL(triggered()), this, SLOT(sortByDate()));

    QActionGroup* sortGroup = new QActionGroup(this);
    sortGroup->addAction(sortByName);
    sortGroup->addAction(sortBySize);
    sortGroup->addAction(sortByDate);

    KToggleAction* sortDescending = new KToggleAction(i18n("Descending"), actionCollection(), "descending");
    connect(sortDescending, SIGNAL(triggered()), this, SLOT(toggleSortOrder()));

    KToggleAction* showHiddenFiles = new KToggleAction(i18n("Show Hidden Files"), actionCollection(), "show_hidden_files");
    //showHiddenFiles->setShortcut(Qt::ALT | Qt::Key_      KDE4-TODO: what Qt-Key represents '.'?
    connect(showHiddenFiles, SIGNAL(triggered()), this, SLOT(showHiddenFiles()));

    KToggleAction* split = new KToggleAction(i18n("Split View"), actionCollection(), "split_view");
    split->setShortcut(Qt::Key_F10);
    split->setIcon(KIcon("view_left_right"));
    connect(split, SIGNAL(triggered()), this, SLOT(toggleSplitView()));

    KAction* reload = new KAction(i18n("Reload"), "F5", actionCollection(), "reload");
    reload->setShortcut(Qt::Key_F5);
    reload->setIcon(KIcon("reload"));
    connect(reload, SIGNAL(triggered()), this, SLOT(reloadView()));

    KAction* stop = new KAction(i18n("Stop"), actionCollection(), "stop");
    stop->setIcon(KIcon("stop"));
    connect(stop, SIGNAL(triggered()), this, SLOT(stopLoading()));

    KToggleAction* showFullLocation = new KToggleAction(i18n("Show Full Location"), actionCollection(), "editable_location");
    showFullLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(showFullLocation, SIGNAL(triggered()), this, SLOT(toggleEditLocation()));

    KToggleAction* editLocation = new KToggleAction(i18n("Edit Location"), actionCollection(), "edit_location");
    editLocation->setShortcut(Qt::Key_F6);
    connect(editLocation, SIGNAL(triggered()), this, SLOT(editLocation()));

    KToggleAction* sidebar = new KToggleAction(i18n("Sidebar"), actionCollection(), "sidebar");
    sidebar->setShortcut(Qt::Key_F9);
    connect(sidebar, SIGNAL(triggered()), this, SLOT(toggleSidebar()));

    KAction* adjustViewProps = new KAction(i18n("Adjust View Properties..."), actionCollection(), "view_properties");
    connect(adjustViewProps, SIGNAL(triggered()), this, SLOT(adjustViewProperties()));

    // setup 'Go' menu
    KStdAction::back(this, SLOT(goBack()), actionCollection());
    KStdAction::forward(this, SLOT(goForward()), actionCollection());
    KStdAction::up(this, SLOT(goUp()), actionCollection());
    KStdAction::home(this, SLOT(goHome()), actionCollection());

    // setup 'Tools' menu
    KAction* openTerminal = new KAction(i18n("Open Terminal"), actionCollection(), "open_terminal");
    openTerminal->setShortcut(Qt::Key_F4);
    openTerminal->setIcon(KIcon("konsole"));
    connect(openTerminal, SIGNAL(triggered()), this, SLOT(openTerminal()));

    KAction* findFile = new KAction(i18n("Find File..."), actionCollection(), "find_file");
    findFile->setShortcut(Qt::Key_F);
    findFile->setIcon(KIcon("filefind"));
    connect(findFile, SIGNAL(triggered()), this, SLOT(findFile()));

    KToggleAction* showFilterBar = new KToggleAction(i18n("Show Filter Bar"), actionCollection(), "show_filter_bar");
    showFilterBar->setShortcut(Qt::Key_Slash);
    connect(showFilterBar, SIGNAL(triggered()), this, SLOT(showFilterBar()));

    KAction* compareFiles = new KAction(i18n("Compare Files"), actionCollection(), "compare_files");
    compareFiles->setIcon(KIcon("kompare"));
    compareFiles->setEnabled(false);
    connect(compareFiles, SIGNAL(triggered()), this, SLOT(compareFiles()));

    // setup 'Settings' menu
    KStdAction::preferences(this, SLOT(editSettings()), actionCollection());
}

void Dolphin::setupCreateNewMenuActions()
{
    // Parts of the following code have been taken
    // from the class KNewMenu located in
    // libqonq/knewmenu.h of Konqueror.
    //  Copyright (C) 1998, 1999 David Faure <faure@kde.org>
    //                2003       Sven Leiber <s.leiber@web.de>

    QStringList files = actionCollection()->instance()->dirs()->findAllResources("templates");
    for (QStringList::Iterator it = files.begin() ; it != files.end(); ++it) {
        if ((*it)[0] != '.' ) {
            KSimpleConfig config(*it, true);
            config.setDesktopGroup();

            // tricky solution to ensure that TextFile is at the beginning
            // because this filetype is the most used (according kde-core discussion)
            const QString name(config.readEntry("Name"));
            QString key(name);

            const QString path(config.readPathEntry("URL"));
            if (!path.endsWith("emptydir")) {
                if (path.endsWith("TextFile.txt")) {
                    key = "1" + key;
                }
                else if (!KDesktopFile::isDesktopFile(path)) {
                    key = "2" + key;
                }
                else if (path.endsWith("URL.desktop")){
                    key = "3" + key;
                }
                else if (path.endsWith("Program.desktop")){
                    key = "4" + key;
                }
                else {
                    key = "5";
                }

                const QString icon(config.readEntry("Icon"));
                const QString comment(config.readEntry("Comment"));
                const QString type(config.readEntry("Type"));

                const QString filePath(*it);


                if (type == "Link") {
                    CreateFileEntry entry;
                    entry.name = name;
                    entry.icon = icon;
                    entry.comment = comment;
                    entry.templatePath = filePath;
                    m_createFileTemplates.insert(key, entry);
                }
            }
        }
    }
    m_createFileTemplates.sort();

    unplugActionList("create_actions");
    KSortableList<CreateFileEntry, QString>::ConstIterator it = m_createFileTemplates.begin();
    KSortableList<CreateFileEntry, QString>::ConstIterator end = m_createFileTemplates.end();
    /* KDE4-TODO:
    while (it != end) {
        CreateFileEntry entry = (*it).value();
        KAction* action = new KAction(entry.name);
        action->setIcon(entry.icon);
        action->setName((*it).index());
        connect(action, SIGNAL(activated()),
                this, SLOT(createFile()));

        const QChar section = ((*it).index()[0]);
        switch (section) {
            case '1':
            case '2': {
                m_fileGroupActions.append(action);
                break;
            }

            case '3':
            case '4': {
                // TODO: not used yet. See documentation of Dolphin::linkGroupActions()
                // and Dolphin::linkToDeviceActions() in the header file for details.
                //m_linkGroupActions.append(action);
                break;
            }

            case '5': {
                // TODO: not used yet. See documentation of Dolphin::linkGroupActions()
                // and Dolphin::linkToDeviceActions() in the header file for details.
                //m_linkToDeviceActions.append(action);
                break;
            }
            default:
                break;
        }
        ++it;
    }

    plugActionList("create_file_group", m_fileGroupActions);
    //plugActionList("create_link_group", m_linkGroupActions);
    //plugActionList("link_to_device", m_linkToDeviceActions);*/
}

void Dolphin::updateHistory()
{
    int index = 0;
    const Q3ValueList<URLNavigator::HistoryElem> list = m_activeView->urlHistory(index);

    KAction* backAction = actionCollection()->action("go_back");
    if (backAction != 0) {
        backAction->setEnabled(index < static_cast<int>(list.count()) - 1);
    }

    KAction* forwardAction = actionCollection()->action("go_forward");
    if (forwardAction != 0) {
        forwardAction->setEnabled(index > 0);
    }
}

void Dolphin::updateEditActions()
{
    const KFileItemList* list = m_activeView->selectedItems();
    if ((list == 0) || (*list).isEmpty()) {
        stateChanged("has_no_selection");
    }
    else {
        stateChanged("has_selection");

        KAction* renameAction = actionCollection()->action("rename");
        if (renameAction != 0) {
            renameAction->setEnabled(list->count() >= 1);
        }

        bool enableMoveToTrash = true;

        KFileItemList::const_iterator it = list->begin();
        const KFileItemList::const_iterator end = list->end();
        while (it != end) {
            KFileItem* item = *it;
            const KUrl& url = item->url();
            // only enable the 'Move to Trash' action for local files
            if (!url.isLocalFile()) {
                enableMoveToTrash = false;
            }
            ++it;
        }

        KAction* moveToTrashAction = actionCollection()->action("move_to_trash");
        moveToTrashAction->setEnabled(enableMoveToTrash);
    }
    updatePasteAction();
}

void Dolphin::updateViewActions()
{
    KAction* zoomInAction = actionCollection()->action(KStdAction::stdName(KStdAction::ZoomIn));
    if (zoomInAction != 0) {
        zoomInAction->setEnabled(m_activeView->isZoomInPossible());
    }

    KAction* zoomOutAction = actionCollection()->action(KStdAction::stdName(KStdAction::ZoomOut));
    if (zoomOutAction != 0) {
        zoomOutAction->setEnabled(m_activeView->isZoomOutPossible());
    }

    KAction* action = 0;
    switch (m_activeView->mode()) {
        case DolphinView::IconsView:
            action = actionCollection()->action("icons");
            break;
        case DolphinView::DetailsView:
            action = actionCollection()->action("details");
            break;
        case DolphinView::PreviewsView:
            action = actionCollection()->action("previews");
            break;
        default:
            break;
    }

    if (action != 0) {
        KToggleAction* toggleAction = static_cast<KToggleAction*>(action);
        toggleAction->setChecked(true);
    }

    slotSortingChanged(m_activeView->sorting());
    slotSortOrderChanged(m_activeView->sortOrder());

    KToggleAction* showFilterBarAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_filter_bar"));
    showFilterBarAction->setChecked(m_activeView->isFilterBarVisible());

    KToggleAction* showHiddenFilesAction =
        static_cast<KToggleAction*>(actionCollection()->action("show_hidden_files"));
    showHiddenFilesAction->setChecked(m_activeView->isShowHiddenFilesEnabled());

    KToggleAction* splitAction = static_cast<KToggleAction*>(actionCollection()->action("split_view"));
    splitAction->setChecked(m_view[SecondaryIdx] != 0);

    KToggleAction* sidebarAction = static_cast<KToggleAction*>(actionCollection()->action("sidebar"));
    sidebarAction->setChecked(m_sidebar != 0);
}

void Dolphin::updateGoActions()
{
    KAction* goUpAction = actionCollection()->action(KStdAction::stdName(KStdAction::Up));
    const KUrl& currentURL = m_activeView->url();
    goUpAction->setEnabled(currentURL.upUrl() != currentURL);
}

void Dolphin::updateViewProperties(const KUrl::List& urls)
{
    if (urls.isEmpty()) {
        return;
    }

    // Updating the view properties might take up to several seconds
    // when dragging several thousand URLs. Writing a KIO slave for this
    // use case is not worth the effort, but at least the main widget
    // must be disabled and a progress should be shown.
    ProgressIndicator progressIndicator(i18n("Updating view properties..."),
                                        QString::null,
                                        urls.count());

    KUrl::List::ConstIterator end = urls.end();
    for(KUrl::List::ConstIterator it = urls.begin(); it != end; ++it) {
        progressIndicator.execOperation();

        ViewProperties props(*it);
        props.save();
    }
}

void Dolphin::copyURLs(const KUrl::List& source, const KUrl& dest)
{
    KIO::Job* job = KIO::copy(source, dest);
    addPendingUndoJob(job, DolphinCommand::Copy, source, dest);
}

void Dolphin::moveURLs(const KUrl::List& source, const KUrl& dest)
{
    KIO::Job* job = KIO::move(source, dest);
    addPendingUndoJob(job, DolphinCommand::Move, source, dest);
}

void Dolphin::addPendingUndoJob(KIO::Job* job,
                                DolphinCommand::Type commandType,
                                const KUrl::List& source,
                                const KUrl& dest)
{
    connect(job, SIGNAL(result(KIO::Job*)),
            this, SLOT(addUndoOperation(KIO::Job*)));

    UndoInfo undoInfo;
    undoInfo.id = job->progressId();
    undoInfo.command = DolphinCommand(commandType, source, dest);
    m_pendingUndoJobs.append(undoInfo);
}

void Dolphin::clearStatusBar()
{
    m_activeView->statusBar()->clear();
}

void Dolphin::openSidebar()
{
    if (m_sidebar != 0) {
        // the sidebar is already open
        return;
    }

    m_sidebar = new Sidebar(m_splitter);
    m_sidebar->show();

    connect(m_sidebar, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(slotURLChangeRequest(const KUrl&)));
    m_splitter->setCollapsible(m_sidebar, false);
    m_splitter->setResizeMode(m_sidebar, QSplitter::KeepSize);
    m_splitter->moveToFirst(m_sidebar);

    SidebarSettings* settings = DolphinSettings::instance().sidebarSettings();
    settings->setVisible(true);
}

#include "dolphin.moc"
