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

#include "commands.h"

#include "kinsertionsort.h"

#include "toplevel.h"
#include "listview.h"

#include <assert.h>

#include <kdebug.h>
#include <klocale.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>

#include <kurldrag.h>
#include <kdesktopfile.h>

CmdGen* CmdGen::s_self = 0;

QString CreateCommand::name() const {
    if (m_separator) {
        return i18n("Insert Separator");
    } else if (m_group) {
        return m_konqi ? i18n("Create Folder") 
            : i18n("Create Folder in Konqueror");
    } else if (!m_originalBookmark.isNull()) {
        return i18n("Copy %1").arg(m_mytext);
    } else {
        return m_konqi ? i18n("Create Bookmark") 
            : i18n("Add Bookmark in Konqueror");
    }
}

void CreateCommand::execute() {
    QString parentAddress = KBookmark::parentAddress(m_to);
    KBookmarkGroup parentGroup = 
        CurrentMgr::bookmarkAt(parentAddress).toGroup();

    QString previousSibling = KBookmark::previousAddress(m_to);

    // kdDebug() << "CreateCommand::execute previousSibling=" 
    //           << previousSibling << endl;
    KBookmark prev = (previousSibling.isEmpty())
        ? KBookmark(QDomElement())
        : CurrentMgr::bookmarkAt(previousSibling);

    KBookmark bk = KBookmark(QDomElement());

    if (m_separator) {
        bk = parentGroup.createNewSeparator();

    } else if (m_group) {
        Q_ASSERT(!m_text.isEmpty());
        bk = parentGroup.createNewFolder(CurrentMgr::self()->mgr(), 
                m_text, false);
        bk.internalElement().setAttribute("folded", (m_open ? "no" : "yes"));
        if (!m_iconPath.isEmpty()) {
            bk.internalElement().setAttribute("icon", m_iconPath);
        }

    } else if (!m_originalBookmark.isNull()) {
        // umm.. moveItem needs bk to be a child already!
        bk = m_originalBookmark;

    } else {
        bk = parentGroup.addBookmark(CurrentMgr::self()->mgr(), 
                m_text, m_url, 
                m_iconPath, false);
    }

    // move to right position
    parentGroup.moveItem(bk, prev);
    if (!(name().isEmpty())) {
        // open the parent (useful if it was empty) - only for manual commands
        parentGroup.internalElement().setAttribute("folded", "no");
    }

    Q_ASSERT(bk.address() == m_to);
}

QString CreateCommand::finalAddress() const {
    Q_ASSERT( !m_to.isEmpty() );
    return m_to;
}

void CreateCommand::unexecute() {
    // kdDebug() << "CreateCommand::unexecute deleting " << m_to << endl;

    KBookmark bk = CurrentMgr::bookmarkAt(m_to);
    Q_ASSERT(!bk.isNull() && !bk.parentGroup().isNull());

    KEBListViewItem *item = 
        static_cast<KEBListViewItem*>(ListView::self()->selectedItems()->first());

    if (item 
            && item->bookmark().hasParent() 
            && item->bookmark().address() == m_to) {
        item->setSelected(false);

        // can't use itemBelow here, in case we're deleting a folder
        QListViewItem *selectItem = item->nextSibling();
        if (!selectItem) {
            // no next sibling ? Go to previous one, then.
            QString selcAddr = bk.parentGroup().previousAddress(bk.address());
            if (selcAddr.isEmpty()) {
                KBookmark::parentAddress(bk.address());
            }
            ListView::self()->setInitialAddress(selcAddr);
        }
    }

    bk.parentGroup().deleteBookmark(bk);
}

/* -------------------------------------- */

QString EditCommand::name() const {
    return i18n("%1 Change").arg(m_mytext);
}

void EditCommand::execute() {
    KBookmark bk = CurrentMgr::bookmarkAt(m_address);
    Q_ASSERT(!bk.isNull());

    m_reverseEditions.clear();

    QValueList<Edition>::Iterator it = m_editions.begin();

    for ( ; it != m_editions.end() ; ++it) {
        // backup current value
        m_reverseEditions.append( Edition((*it).attr, 
                    bk.internalElement().attribute((*it).attr)));
        // set new value
        bk.internalElement().setAttribute((*it).attr, (*it).value);
    }
}

void EditCommand::unexecute() {
    // code reuse
    EditCommand cmd(m_address, m_reverseEditions);
    cmd.execute();
    // get the editions back from it, 
    // in case they changed 
    // (hmm, shouldn't happen - TODO CHECK!)
    m_editions = cmd.m_reverseEditions;
}

/* -------------------------------------- */

QString NodeEditCommand::name() const {
    // TODO - make dynamic
    return i18n("Renaming"); 
}

QString NodeEditCommand::getNodeText(KBookmark bk, const QStringList &nodehier) {
    QDomNode subnode = bk.internalElement();
    for (QStringList::ConstIterator it = nodehier.begin(); 
            it != nodehier.end(); ++it)
    {
        subnode = subnode.namedItem((*it));
        if (subnode.isNull())
            return QString::null;
    }
    return (subnode.firstChild().isNull())
         ? QString::null
         : subnode.firstChild().toText().data();
}

QString NodeEditCommand::setNodeText(KBookmark bk, const QStringList &nodehier,
                                     const QString newValue) {
    QDomNode subnode = bk.internalElement();
    for (QStringList::ConstIterator it = nodehier.begin(); 
            it != nodehier.end(); ++it)
    {
        subnode = subnode.namedItem((*it));
        if (subnode.isNull()) {
            subnode = bk.internalElement().ownerDocument().createElement((*it));
            bk.internalElement().appendChild(subnode);
        }
    }

    if (subnode.firstChild().isNull()) {
        QDomText domtext = subnode.ownerDocument().createTextNode("");
        subnode.appendChild(domtext);
    }

    QDomText domtext = subnode.firstChild().toText();

    QString oldText = domtext.data();
    domtext.setData(newValue);
    return oldText;
}

void NodeEditCommand::execute() {
    // DUPLICATED HEAVILY FROM KIO/BOOKMARKS
    KBookmark bk = CurrentMgr::bookmarkAt(m_address);
    Q_ASSERT(!bk.isNull());
    m_oldText = setNodeText(bk, QStringList() << m_nodename, m_newText);
}

void NodeEditCommand::unexecute() {
    // reuse code 
    NodeEditCommand cmd(m_address, m_oldText, m_nodename);
    cmd.execute();
    // get the old text back from it, in case they changed 
    // (hmm, shouldn't happen)
    // AK - DUP'ed from above???
    m_newText = cmd.m_oldText;
}

/* -------------------------------------- */

void DeleteCommand::execute() {
    // kdDebug() << "DeleteCommand::execute " << m_from << endl;

    KBookmark bk = CurrentMgr::bookmarkAt(m_from);
    Q_ASSERT(!bk.isNull());

    if (m_contentOnly) {
        QDomElement groupRoot = bk.internalElement();

        QDomNode n = groupRoot.firstChild();
        while (!n.isNull()) {
            QDomElement e = n.toElement();
            if (!e.isNull()) {
                // kdDebug() << e.tagName() << endl;
            }
            QDomNode next = n.nextSibling();
            groupRoot.removeChild(n);
            n = next;
        }
        return;
    }

    // TODO - bug - unparsed xml is lost after undo, 
    //              we must store it all therefore
    if (!m_cmd) {
        if (bk.isGroup()) {
            m_cmd = new CreateCommand(
                    m_from, bk.fullText(), bk.icon(),
                    bk.internalElement().attribute("folded") == "no");
            m_subCmd = deleteAll(bk.toGroup());
            m_subCmd->execute();

        } else {
            m_cmd = (bk.isSeparator())
                ? new CreateCommand(m_from)
                : new CreateCommand(m_from, bk.fullText(), 
                        bk.icon(), bk.url());
        }
    }

    m_cmd->unexecute();
}

void DeleteCommand::unexecute() {
    // kdDebug() << "DeleteCommand::unexecute " << m_from << endl;

    if (m_contentOnly) {
        // TODO - recover saved metadata
        return;
    }

    m_cmd->execute();

    if (m_subCmd) {
        m_subCmd->unexecute();
    }
}

KMacroCommand* DeleteCommand::deleteAll(const KBookmarkGroup & parentGroup) {
    KMacroCommand *cmd = new KMacroCommand(QString::null);
    QStringList lstToDelete;
    // we need to delete from the end, to avoid index shifting
    for (KBookmark bk = parentGroup.first(); 
            !bk.isNull(); bk = parentGroup.next(bk))
        lstToDelete.prepend(bk.address());
    for (QStringList::Iterator it = lstToDelete.begin(); 
            it != lstToDelete.end(); ++it)
        cmd->addCommand(new DeleteCommand((*it)));
    return cmd;
}

/* -------------------------------------- */

QString MoveCommand::name() const {
    return i18n("Move %1").arg(m_mytext);
}

void MoveCommand::execute() {
    // kdDebug() << "MoveCommand::execute moving from=" << m_from 
    //           << " to=" << m_to << endl;

    KBookmark bk = CurrentMgr::bookmarkAt(m_from);
    Q_ASSERT(!bk.isNull());

    // look for m_from in the QDom tree
    KBookmark oldParent = 
        CurrentMgr::bookmarkAt(KBookmark::parentAddress(m_from));
    bool wasFirstChild = (KBookmark::positionInParent(m_from) == 0);

    KBookmark oldPreviousSibling = wasFirstChild
        ? KBookmark(QDomElement())
        : CurrentMgr::bookmarkAt(
                KBookmark::previousAddress(m_from));

    // look for m_to in the QDom tree
    QString parentAddress = KBookmark::parentAddress(m_to);

    KBookmark newParent = CurrentMgr::bookmarkAt(parentAddress);
    Q_ASSERT(!newParent.isNull());
    Q_ASSERT(newParent.isGroup());

    bool isFirstChild = (KBookmark::positionInParent(m_to) == 0);

    if (isFirstChild) {
        newParent.toGroup().moveItem(bk, QDomElement());

    } else {
        QString afterAddress = KBookmark::previousAddress(m_to);

        // kdDebug() << "MoveCommand::execute afterAddress=" 
        //           << afterAddress << endl;
        KBookmark afterNow = CurrentMgr::bookmarkAt(afterAddress);
        Q_ASSERT(!afterNow.isNull());

        bool movedOkay = newParent.toGroup().moveItem(bk, afterNow);
        Q_ASSERT(movedOkay);

        // kdDebug() << "MoveCommand::execute after moving in the dom tree"
        //              ": item=" << bk.address() << endl;
    }

    // because we moved stuff around, the from/to 
    // addresses can have changed, update
    m_to = bk.address();
    m_from = (wasFirstChild)
        ? (oldParent.address() + "/0")
        : KBookmark::nextAddress(oldPreviousSibling.address());
    // kdDebug() << "MoveCommand::execute : new addresses from=" 
    //           << m_from << " to=" << m_to << endl;
}

QString MoveCommand::finalAddress() const {
    Q_ASSERT( !m_to.isEmpty() );
    return m_to;
}

void MoveCommand::unexecute() {
    // let's not duplicate code.
    MoveCommand undoCmd(m_to, m_from);
    undoCmd.execute();
    // get the addresses back from that command, in case they changed
    m_from = undoCmd.m_to;
    m_to = undoCmd.m_from;
}

/* -------------------------------------- */

class SortItem {
    public:
        SortItem(const KBookmark & bk) : m_bk(bk) { ; } 

        bool operator == (const SortItem & s) { 
            return (m_bk.internalElement() == s.m_bk.internalElement()); }

        bool isNull() const { 
            return m_bk.isNull(); }

        SortItem previousSibling() const { 
            return m_bk.parentGroup().previous(m_bk); }

        SortItem nextSibling() const { 
            return m_bk.parentGroup().next(m_bk); }

        const KBookmark& bookmark() const { 
            return m_bk; }

    private:
        KBookmark m_bk;
};

class SortByName {
    public:
        static QString key(const SortItem &item) { 
            return (item.bookmark().isGroup() ? "a" : "b") 
                + (item.bookmark().fullText().lower()); 
        }
};

/* -------------------------------------- */

void SortCommand::execute() {
    if (m_commands.isEmpty()) {
        KBookmarkGroup grp = CurrentMgr::bookmarkAt(m_groupAddress).toGroup();
        Q_ASSERT(!grp.isNull());
        SortItem firstChild(grp.first());
        // this will call moveAfter, which will add 
        // the subcommands for moving the items
        kInsertionSort<SortItem, SortByName, QString, SortCommand>
            (firstChild, (*this));

    } else {
        // don't execute for second time on addCommand(cmd)
        KMacroCommand::execute();
    }
}

void SortCommand::moveAfter(const SortItem &moveMe, 
        const SortItem &afterMe) {
    QString destAddress = 
        afterMe.isNull() 
        // move as first child
        ? KBookmark::parentAddress(moveMe.bookmark().address()) + "/0"   
        // move after "afterMe"
        : KBookmark::nextAddress(afterMe.bookmark().address());          

    MoveCommand *cmd = new MoveCommand(moveMe.bookmark().address(), 
            destAddress);
    cmd->execute();
    this->addCommand(cmd);
}

void SortCommand::unexecute() {
    KMacroCommand::unexecute();
}

/* -------------------------------------- */

KMacroCommand* CmdGen::setAsToolbar(const KBookmark &bk) {
    KMacroCommand *mcmd = new KMacroCommand(i18n("Set as Bookmark Toolbar"));

    KBookmarkGroup oldToolbar = CurrentMgr::self()->mgr()->toolbar();
    if (!oldToolbar.isNull()) {
        QValueList<EditCommand::Edition> lst;
        lst.append(EditCommand::Edition("toolbar", "no"));
        lst.append(EditCommand::Edition("icon", ""));
        EditCommand *cmd1 = new EditCommand(oldToolbar.address(), lst);
        mcmd->addCommand(cmd1);
    }

    QValueList<EditCommand::Edition> lst;
    lst.append(EditCommand::Edition("toolbar", "yes"));
    lst.append(EditCommand::Edition("icon", "bookmark_toolbar"));
    // TODO - see below
    EditCommand *cmd2 = new EditCommand(bk.address(), lst);
    mcmd->addCommand(cmd2);

    return mcmd;
}

bool CmdGen::shownInToolbar(const KBookmark &bk) {
    return (bk.internalElement().attribute("showintoolbar") == "yes");
}

KMacroCommand* CmdGen::setShownInToolbar(const KBookmark &bk, bool show) {
    QString i18n_name = i18n("%1 in Bookmark Toolbar").arg(show ? i18n("Show") 
            : i18n("Hide"));
    KMacroCommand *mcmd = new KMacroCommand(i18n_name);

    QValueList<EditCommand::Edition> lst;
    lst.append(EditCommand::Edition("showintoolbar", show ? "yes" : "no"));
    EditCommand *cmd2 = new EditCommand(bk.address(), lst);
    mcmd->addCommand(cmd2);

    return mcmd;
}

KMacroCommand* CmdGen::deleteItems(const QString &commandName, 
        QPtrList<KEBListViewItem> *items) {
    QPtrListIterator<KEBListViewItem> it(*items);
    KMacroCommand *mcmd = new KMacroCommand(commandName);
    for (; it.current() != 0; ++it) {
        DeleteCommand *dcmd = 
            new DeleteCommand(it.current()->bookmark().address());
        dcmd->execute();
        mcmd->addCommand(dcmd);
    }
    return mcmd;
}


KMacroCommand* CmdGen::insertMimeSource(
    const QString &cmdName, QMimeSource *_data, const QString &addr
) {
    QMimeSource *data = _data;
    bool modified = false;
    const char *format = 0;
    for (int i = 0; format = data->format(i), format; i++) {
        // qt docs don't say if encodedData(blah) where
        // blah is not a stored mimetype should return null
        // or not. so, we search. sucky...
        if (strcmp(format, "GALEON_BOOKMARK") == 0) { 
            modified = true;
            QStoredDrag *mydrag = new QStoredDrag("application/x-xbel");
            mydrag->setEncodedData(data->encodedData("GALEON_BOOKMARK"));
            data = mydrag;
            break;
        } else if( strcmp(format, "application/x-xbel" )==0) {
            /* nothing we created a kbbookmarks drag when we copy element (slotcopy/slotpaste)*/
            break;
        } else if (strcmp(format, "text/uri-list") == 0) { 
            KURL::List uris;
            if (!KURLDrag::decode(data, uris))
                continue; // break out of format loop
            KURL::List::ConstIterator uit = uris.begin();
            KURL::List::ConstIterator uEnd = uris.end();
            QValueList<KBookmark> urlBks;
            for ( ; uit != uEnd ; ++uit ) {
                if (!(*uit).url().endsWith(".desktop"))  {
                    urlBks << KBookmark::standaloneBookmark((*uit).prettyURL(), (*uit));
                    continue;
                }
                KDesktopFile df((*uit).path(), true);
                QString title = df.readName();
                KURL url(df.readURL());
                if (title.isNull())
                    title = url.prettyURL();
                urlBks << KBookmark::standaloneBookmark(title, url, df.readIcon());
            }
            KBookmarkDrag *mydrag = KBookmarkDrag::newDrag(urlBks, 0);
            data = mydrag;
        }
    }
    if (!KBookmarkDrag::canDecode(data))
        return 0;
    KMacroCommand *mcmd = new KMacroCommand(cmdName);
    QString currentAddress = addr;
    QValueList<KBookmark> bookmarks = KBookmarkDrag::decode(data);
    for (QValueListConstIterator<KBookmark> it = bookmarks.begin(); 
            it != bookmarks.end(); ++it) {
        CreateCommand *cmd = new CreateCommand(currentAddress, (*it));
        cmd->execute();
        mcmd->addCommand(cmd);
        currentAddress = KBookmark::nextAddress(currentAddress);
    }
    if (modified)
        delete data;
    return mcmd;
}

KMacroCommand* CmdGen::itemsMoved(QPtrList<KEBListViewItem> *items, 
        const QString &newAddress, bool copy) {
    KMacroCommand *mcmd = new KMacroCommand(copy ? i18n("Copy Items") 
            : i18n("Move Items"));

    QString bkInsertAddr = newAddress;

    for (QPtrListIterator<KEBListViewItem> it(*items); 
            it.current() != 0; ++it) {
        if (copy) {
            CreateCommand *cmd;
            cmd = new CreateCommand(
                    bkInsertAddr,
                    (*it)->bookmark().internalElement()
                    .cloneNode(true).toElement(),
                    (*it)->bookmark().text());

            cmd->execute();
            mcmd->addCommand(cmd);

            bkInsertAddr = cmd->finalAddress();

        } else /* if (move) */ {
            QString oldAddress = (*it)->bookmark().address();
            if (bkInsertAddr.startsWith(oldAddress))
                continue;

            MoveCommand *cmd = new MoveCommand(oldAddress, bkInsertAddr,
                    (*it)->bookmark().text());
            cmd->execute();
            mcmd->addCommand(cmd);

            bkInsertAddr = cmd->finalAddress();
        }

        bkInsertAddr = KBookmark::nextAddress(bkInsertAddr);
    }

    return mcmd;
}
