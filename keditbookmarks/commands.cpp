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

#include <assert.h>

#include <kdebug.h>
#include <klocale.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>

#include "kinsertionsort.h"

#include "toplevel.h"
#include "listview.h"

#include "commands.h"

CmdGen* CmdGen::s_self = 0;

QString CreateCommand::name() const {
   if (m_separator) {
      return i18n("Insert Separator");
   } else if (m_group) {
      return m_konqi ? i18n("Create Folder") : i18n("Create Folder in Konqueror");
   } else if (!m_originalBookmark.isNull()) {
      return i18n("Copy %1").arg(m_mytext);
   } else {
      return m_konqi ? i18n("Create Bookmark") : i18n("Add Bookmark in Konqueror");
   }
}

void CreateCommand::execute() {
   QString parentAddress = KBookmark::parentAddress(m_to);
   KBookmarkGroup parentGroup = CurrentMgr::bookmarkAt(parentAddress).toGroup();

   QString previousSibling = KBookmark::previousAddress(m_to);

   // kdDebug() << "CreateCommand::execute previousSibling=" << previousSibling << endl;
   KBookmark prev = 
        (previousSibling.isEmpty())
      ? KBookmark(QDomElement())
      : CurrentMgr::bookmarkAt(previousSibling);

   KBookmark bk = KBookmark(QDomElement());

   if (m_separator) {
      bk = parentGroup.createNewSeparator();

   } else if (m_group) {
      Q_ASSERT(!m_text.isEmpty());
      bk = parentGroup.createNewFolder(CurrentMgr::self()->mgr(), m_text, false);
      bk.internalElement().setAttribute("folded", (m_open ? "no" : "yes"));
      if (!m_iconPath.isEmpty()) {
         bk.internalElement().setAttribute("icon", m_iconPath);
      }

   } else if (!m_originalBookmark.isNull()) {
      // umm.. moveItem needs bk to be a child already!
      bk = m_originalBookmark;

   } else {
      bk = parentGroup.addBookmark(CurrentMgr::self()->mgr(), m_text, m_url, m_iconPath, false);
   }

   // move to right position
   parentGroup.moveItem(bk, prev);
   if (!(name().isEmpty())) {
      // open the parent (useful if it was empty) - only for manual commands
      parentGroup.internalElement().setAttribute("folded", "no");
   }
   
   Q_ASSERT(bk.address() == m_to);
}

QString CreateCommand::finalAddress() {
   Q_ASSERT( !m_to.isEmpty() );
   return m_to;
}

void CreateCommand::unexecute() {
   // kdDebug() << "CreateCommand::unexecute deleting " << m_to << endl;

   KBookmark bk = CurrentMgr::bookmarkAt(m_to);
   Q_ASSERT(!bk.isNull() && !bk.parentGroup().isNull());

   KEBListViewItem *item = 
      static_cast<KEBListViewItem*>(ListView::self()->firstSelected());

   if (item && item->bookmark().hasParent() && item->bookmark().address() == m_to) {
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
      m_reverseEditions.append( Edition((*it).attr, bk.internalElement().attribute((*it).attr)));
      // set new value
      bk.internalElement().setAttribute((*it).attr, (*it).value);
   }
}

void EditCommand::unexecute() {
   // code reuse
   EditCommand cmd(m_address, m_reverseEditions);
   cmd.execute();
   // get the editions back from it, in case they changed (hmm, shouldn't happen - TODO CHECK!)
   m_editions = cmd.m_reverseEditions;
}

/* -------------------------------------- */

QString NodeEditCommand::name() const {
   // TODO - make dynamic
   return i18n("Renaming"); 
}

QString NodeEditCommand::getNodeText(KBookmark bk, const QString &nodename) {
   QDomNode subnode = bk.internalElement().namedItem(nodename);
   return subnode.isNull() || subnode.firstChild().isNull() 
        ? QString::null
        : subnode.firstChild().toText().data();
}

void NodeEditCommand::execute() {
   KBookmark bk = CurrentMgr::bookmarkAt(m_address);
   Q_ASSERT(!bk.isNull());

   QDomNode subnode = bk.internalElement().namedItem(m_nodename);
   if (subnode.isNull()) {
      subnode = bk.internalElement().ownerDocument().createElement(m_nodename);
      bk.internalElement().appendChild(subnode);
   }

   if (subnode.firstChild().isNull()) {
      QDomText domtext = subnode.ownerDocument().createTextNode("");
      subnode.appendChild(domtext);
   }

   QDomText domtext = subnode.firstChild().toText();

   m_oldText = domtext.data();
   domtext.setData(m_newText);
}

void NodeEditCommand::unexecute() {
   // code reuse
   NodeEditCommand cmd(m_address, m_oldText, m_nodename);
   cmd.execute();
   // get the old text back from it, in case they changed (hmm, shouldn't happen)
   m_newText = cmd.m_oldText;
}

/* -------------------------------------- */

void DeleteCommand::execute() {
   // kdDebug() << "DeleteCommand::execute " << m_from << endl;

   KBookmark bk = CurrentMgr::bookmarkAt(m_from);
   Q_ASSERT(!bk.isNull());

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
               : new CreateCommand(m_from, bk.fullText(), bk.icon(), bk.url());
      }
   }

   m_cmd->unexecute();
}

void DeleteCommand::unexecute() {
   m_cmd->execute();

   if (m_subCmd) {
      m_subCmd->unexecute();
   }
}

KMacroCommand* DeleteCommand::deleteAll(const KBookmarkGroup & parentGroup) {
   KMacroCommand *cmd = new KMacroCommand(QString::null);
   QStringList lstToDelete;
   // we need to delete from the end, to avoid index shifting
   for (KBookmark bk = parentGroup.first(); !bk.isNull(); bk = parentGroup.next(bk)) {
      lstToDelete.prepend(bk.address());
   }
   for (QStringList::Iterator it = lstToDelete.begin(); it != lstToDelete.end(); ++it) {
      cmd->addCommand(new DeleteCommand((*it)));
   }
   // TODO - remove all metadata here???
   return cmd;
}

/* -------------------------------------- */

QString MoveCommand::name() const {
   return i18n("Move %1").arg(m_mytext);
}

void MoveCommand::execute() {
   // kdDebug() << "MoveCommand::execute moving from=" << m_from << " to=" << m_to << endl;

   KBookmark bk = CurrentMgr::bookmarkAt(m_from);
   Q_ASSERT(!bk.isNull());

   // look for m_from in the QDom tree
   KBookmark oldParent = CurrentMgr::bookmarkAt(KBookmark::parentAddress(m_from));

   bool wasFirstChild = (KBookmark::positionInParent(m_from) == 0);
   KBookmark oldPreviousSibling = 
                 (wasFirstChild)
               ? KBookmark(QDomElement())
               : CurrentMgr::bookmarkAt(KBookmark::previousAddress(m_from));

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

      // kdDebug() << "MoveCommand::execute afterAddress=" << afterAddress << endl;
      KBookmark afterNow = CurrentMgr::bookmarkAt(afterAddress);
      Q_ASSERT(!afterNow.isNull());

      bool movedOkay = newParent.toGroup().moveItem(bk, afterNow);
      Q_ASSERT(movedOkay);

      // kdDebug() << "MoveCommand::execute after moving in the dom tree : item=" << bk.address() << endl;
   }

   // because we moved stuff around, the addresses from/to can have changed, update
   m_to = bk.address();
   m_from = (wasFirstChild)
          ? (oldParent.address() + "/0")
          : KBookmark::nextAddress(oldPreviousSibling.address());
   // kdDebug() << "MoveCommand::execute : new addresses from=" << m_from << " to=" << m_to << endl;
}

QString MoveCommand::finalAddress() {
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
      return (item.bookmark().isGroup() ? "a" : "b") + item.bookmark().fullText().lower(); 
   }
};

/* -------------------------------------- */

void SortCommand::execute() {
   if (m_commands.isEmpty()) {
      KBookmarkGroup grp = CurrentMgr::bookmarkAt(m_groupAddress).toGroup();
      Q_ASSERT(!grp.isNull());
      SortItem firstChild(grp.first());
      // this will call moveAfter, which will add the subcommands for moving the items
      kInsertionSort<SortItem, SortByName, QString, SortCommand>(firstChild, (*this));

   } else {
      // don't execute for second time on addCommand(cmd)
      KMacroCommand::execute();
   }
}

void SortCommand::moveAfter(const SortItem &moveMe, const SortItem &afterMe) {
   QString destAddress = 
      afterMe.isNull() 
    ? KBookmark::parentAddress(moveMe.bookmark().address()) + "/0"   // move as first child
    : KBookmark::nextAddress(afterMe.bookmark().address());          // move after "afterMe"

   MoveCommand *cmd = new MoveCommand(moveMe.bookmark().address(), destAddress);
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

KMacroCommand* CmdGen::showInToolbar(const KBookmark &bk) {
   KMacroCommand *mcmd = new KMacroCommand(i18n("Show in Bookmark Toolbar"));

   QValueList<EditCommand::Edition> lst;
   lst.append(EditCommand::Edition("showintoolbar", "yes"));
   // TODO - some proper kind of feedback - e.g, an icon effect/emblem
   //      - the following will erase the stored icon for the item
   lst.append(EditCommand::Edition("icon", "bookmark_toolbar"));
   EditCommand *cmd2 = new EditCommand(bk.address(), lst);
   mcmd->addCommand(cmd2);

   return mcmd;
}

KMacroCommand* CmdGen::deleteItems(const QString &commandName, QPtrList<KEBListViewItem> *items) {
   QPtrListIterator<KEBListViewItem> it(*items);
   KMacroCommand *mcmd = new KMacroCommand(commandName);
   for (; it.current() != 0; ++it) {
      DeleteCommand *dcmd = new DeleteCommand(it.current()->bookmark().address());
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
      }
   }
   if (!KBookmarkDrag::canDecode(data)) {
      return 0;
   }
   KMacroCommand *mcmd = new KMacroCommand(cmdName);
   QString currentAddress = addr;
   QValueList<KBookmark> bookmarks = KBookmarkDrag::decode(data);
   for (QValueListConstIterator<KBookmark> it = bookmarks.begin(); it != bookmarks.end(); ++it) {
      CreateCommand *cmd = new CreateCommand(currentAddress, (*it));
      cmd->execute();
      mcmd->addCommand(cmd);
      // kdDebug() << "CmdGen::insertMimeSource url=" 
      //           << (*it).url().prettyURL() << currentAddress << endl;
      currentAddress = KBookmark::nextAddress(currentAddress);
   }
   if (modified) {
      delete data;
   }
   return mcmd;
}

KMacroCommand* CmdGen::itemsMoved(QPtrList<KEBListViewItem> *items, const QString &newAddress, bool copy) {
   KMacroCommand *mcmd = new KMacroCommand(copy ? i18n("Copy Items") : i18n("Move Items"));

   QString bkInsertAddr = newAddress;

   for (QPtrListIterator<KEBListViewItem> it(*items); it.current() != 0; ++it) {
      KCommand *cmd;
      if (copy) {
         cmd = new CreateCommand(
                     bkInsertAddr,
                     (*it)->bookmark().internalElement().cloneNode(true).toElement(),
                     (*it)->bookmark().text());

      } else /* if (move) */ {
         QString oldAddress = (*it)->bookmark().address();

         if (bkInsertAddr.startsWith(oldAddress)) {
            continue;
         }

         cmd = new MoveCommand(oldAddress, bkInsertAddr,
                               (*it)->bookmark().text());
      }

      cmd->execute();
      mcmd->addCommand(cmd);
      
      FinalAddressCommand *addrcmd = dynamic_cast<FinalAddressCommand*>(cmd);
      assert(addrcmd);
      bkInsertAddr = KBookmark::nextAddress(addrcmd->finalAddress());
   }

   return mcmd;
}
