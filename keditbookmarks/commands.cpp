/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002 Alexander Kellett <lypanov@kde.org>

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

#include <qregexp.h>
#include <kdebug.h>
#include <klocale.h>
#include <krfcdate.h>

#include <kcharsets.h>

#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>

#include "commands.h"
#include "toplevel.h"
#include "kinsertionsort.h"

void MoveCommand::execute()
{
   kdDebug() << "MoveCommand::execute moving from=" << m_from << " to=" << m_to << endl;

   // Look for m_from in the QDom tree
   KBookmark bk = KEBTopLevel::bookmarkManager()->findByAddress( m_from );
   Q_ASSERT( !bk.isNull() );
   //kdDebug() << "BEFORE:" << KEBTopLevel::bookmarkManager()->internalDocument().toString() << endl;
   int posInOldParent = KBookmark::positionInParent( m_from );
   KBookmark oldParent = KEBTopLevel::bookmarkManager()->findByAddress( KBookmark::parentAddress( m_from ) );
   KBookmark oldPreviousSibling = posInOldParent == 0 ? KBookmark(QDomElement())
      : KEBTopLevel::bookmarkManager()->findByAddress( KBookmark::previousAddress( m_from ) );

   // Look for m_to in the QDom tree (as parent address and position in parent)
   int posInNewParent = KBookmark::positionInParent( m_to );
   QString parentAddress = KBookmark::parentAddress( m_to );
   //kdDebug() << "MoveCommand::execute parentAddress=" << parentAddress << " posInNewParent=" << posInNewParent << endl;
   KBookmark newParentBk = KEBTopLevel::bookmarkManager()->findByAddress( parentAddress );
   Q_ASSERT( !newParentBk.isNull() );
   Q_ASSERT( newParentBk.isGroup() );

   if ( posInNewParent == 0 ) {
      // First child
      newParentBk.toGroup().moveItem( bk, QDomElement() );

   } else {
      QString afterAddress = KBookmark::previousAddress( m_to );
      kdDebug() << "MoveCommand::execute afterAddress=" << afterAddress << endl;
      KBookmark afterNow = KEBTopLevel::bookmarkManager()->findByAddress( afterAddress );
      Q_ASSERT(!afterNow.isNull());
      bool result = newParentBk.toGroup().moveItem( bk, afterNow );
      Q_ASSERT(result);
      kdDebug() << "MoveCommand::execute after moving in the dom tree : item=" << bk.address() << endl;
   }

   // Ok, now this is the most tricky bit.
   // Because we moved stuff around, the addresses from/to can have changed
   // So we look into the dom tree to get the new positions, using a reference
   // The reference is :
   if ( posInOldParent == 0 ) {
      // the old parent, if we were the first child
      m_from = oldParent.address() + "/0";
   } else {
      // otherwise the previous sibling
      m_from = KBookmark::nextAddress( oldPreviousSibling.address() );
   }
   m_to = bk.address();
   kdDebug() << "MoveCommand::execute : new addresses from=" << m_from << " to=" << m_to << endl;
}

QString MoveCommand::finalAddress()
{
   // AK - assert( alreadyExecuted ); ???
   return m_to;
}

void MoveCommand::unexecute()
{
   // Let's not duplicate code.
   MoveCommand undoCmd( QString::null, m_to, m_from );
   undoCmd.execute();
   // Get the addresses back from that command, in case they changed
   m_from = undoCmd.m_to;
   m_to = undoCmd.m_from;
}

void CreateCommand::execute()
{

   kdDebug() << "CreateCommand::execute-ing " << this << endl;

   // Gather some info
   QString parentAddress = KBookmark::parentAddress( m_to );
   KBookmarkGroup parentGroup = KEBTopLevel::bookmarkManager()->findByAddress( parentAddress ).toGroup();
   QString previousSibling = KBookmark::previousAddress( m_to );
   //kdDebug() << "CreateCommand::execute previousSibling=" << previousSibling << endl;
   KBookmark prev = previousSibling.isEmpty() ? KBookmark(QDomElement())
      : KEBTopLevel::bookmarkManager()->findByAddress( previousSibling );

   // Create
   KBookmark bk = KBookmark(QDomElement());
   if (m_originalBookmark.isNull()) {
      if (m_separator) {
         bk = parentGroup.createNewSeparator();
      } else {
         if (m_group) {
            Q_ASSERT( !m_text.isEmpty() );
            bk = parentGroup.createNewFolder( KEBTopLevel::bookmarkManager(), m_text, false );

            kdDebug() << "CreateCommand::execute " << m_group << " open : " << m_open << endl;
            bk.internalElement().setAttribute( "folded", m_open ? "no" : "yes" );
            if(!m_iconPath.isEmpty()) {
               bk.internalElement().setAttribute( "icon",m_iconPath );
            }
         } else {
            bk = parentGroup.addBookmark( KEBTopLevel::bookmarkManager(), m_text, m_url, m_iconPath, false);
         }
      }
   } else {
      bk = m_originalBookmark;
   }

   // Move to right position
   parentGroup.moveItem( bk, prev );
   if ( !name().isEmpty() ) {
      kdDebug() << "Opening parent" << endl;
      // Open the parent (useful if it was empty) - only for manual commands
      parentGroup.internalElement().setAttribute( "folded", "no" );
   }
   Q_ASSERT( bk.address() == m_to );
}

QString CreateCommand::finalAddress()
{
   // AK - assert( hasBeenExecuted ); ???
   return m_to;
}

void CreateCommand::unexecute()
{
   kdDebug() << "CreateCommand::unexecute deleting " << m_to << endl;
   KBookmark bk = KEBTopLevel::bookmarkManager()->findByAddress( m_to );
   Q_ASSERT( !bk.isNull() );
   Q_ASSERT( !bk.parentGroup().isNull() );
   // Update GUI
   // but first we need to select a valid item
   KListView * lv = KEBTopLevel::self()->listView();
   KEBListViewItem * item = static_cast<KEBListViewItem*>(lv->selectedItem());
   if (item && item->bookmark().address() == m_to) {
      lv->setSelected(item,false);
      // can't use itemBelow here, in case we're deleting a folder
      if ( item->nextSibling() ) {
         lv->setSelected( item->nextSibling(), true );

      } else {
         // No next sibling ? Go to previous one, then.
         QString prevAddr = bk.parentGroup().previousAddress( bk.address() );
         if ( !prevAddr.isEmpty() ) {
            QListViewItem * prev = KEBTopLevel::self()->findByAddress( prevAddr );
            if (prev) {
               lv->setSelected( prev, true );
            }
         } else {
            // No previous sibling either ? Go up, then.
            QListViewItem * par = KEBTopLevel::self()->findByAddress( KBookmark::parentAddress( bk.address() ) );
            if (par) {
               lv->setSelected( par, true );
            }
         }
      }
   }

   bk.parentGroup().deleteBookmark( bk );
}

void DeleteCommand::execute()
{
   kdDebug() << "DeleteCommand::execute " << m_from << endl;
   KBookmark bk = KEBTopLevel::bookmarkManager()->findByAddress( m_from );
   Q_ASSERT(!bk.isNull());
   if ( !m_cmd ) {
      if ( bk.isGroup() ) {
         m_cmd = new CreateCommand(QString::null, m_from, bk.fullText(),bk.icon(),
               bk.internalElement().attribute("folded")=="no");
         m_subCmd = deleteAll( bk.toGroup() );
         m_subCmd->execute();
      } else {
         if ( bk.isSeparator() ) {
            m_cmd = new CreateCommand(QString::null, m_from );
         } else {
            m_cmd = new CreateCommand(QString::null, m_from, bk.fullText(),bk.icon(),bk.url());
         }
      }
   }

   m_cmd->unexecute();
}

void DeleteCommand::unexecute()
{
   m_cmd->execute();
   if (m_subCmd) {
      m_subCmd->unexecute();
   }
}

KMacroCommand * DeleteCommand::deleteAll( const KBookmarkGroup & parentGroup )
{
   KMacroCommand * cmd = new KMacroCommand( QString::null );
   QStringList lstToDelete;
   // We need to delete from the end, to avoid index shifting
   for ( KBookmark bk = parentGroup.first() ; !bk.isNull() ; bk = parentGroup.next(bk) ) {
      lstToDelete.prepend( bk.address() );
   }
   for ( QStringList::Iterator it = lstToDelete.begin(); it != lstToDelete.end() ; ++it ) {
      kdDebug() << "DeleteCommand::deleteAll: deleting " << *it << endl;
      cmd->addCommand( new DeleteCommand( QString::null, *it ) );
   }
   return cmd;
}

void EditCommand::execute()
{
   KBookmark bk = KEBTopLevel::bookmarkManager()->findByAddress( m_address );
   Q_ASSERT( !bk.isNull() );
   m_reverseEditions.clear();
   QValueList<Edition>::Iterator it = m_editions.begin();
   for ( ; it != m_editions.end() ; ++it ) {
      // backup current value
      m_reverseEditions.append( Edition((*it).attr, bk.internalElement().attribute((*it).attr)) );
      // set new value
      bk.internalElement().setAttribute( (*it).attr, (*it).value );
   }
}

void EditCommand::unexecute()
{
   // Let's not duplicate code.
   EditCommand cmd( QString::null, m_address, m_reverseEditions );
   cmd.execute();
   // Get the editions back from it, in case they changed (hmm, shouldn't happen)
   m_editions = cmd.m_reverseEditions;
}

void RenameCommand::execute()
{
   KBookmark bk = KEBTopLevel::bookmarkManager()->findByAddress( m_address );
   Q_ASSERT( !bk.isNull() );

   QDomNode titleNode = bk.internalElement().namedItem("title");
   Q_ASSERT( !titleNode.isNull() );
   if ( titleNode.firstChild().isNull() ) {
      // no text child yet
      QDomText domtext = titleNode.ownerDocument().createTextNode( "" );
      titleNode.appendChild( domtext );
   }
   QDomText domtext = titleNode.firstChild().toText();

   m_oldText = domtext.data();
   domtext.setData( m_newText );
}

void RenameCommand::unexecute()
{
   // Let's not duplicate code.
   RenameCommand cmd( QString::null, m_address, m_oldText );
   cmd.execute();
   // Get the old text back from it, in case they changed (hmm, shouldn't happen)
   m_newText = cmd.m_oldText;
}

/////////////

class SortItem
{
public:
   SortItem( const KBookmark & bk ) : m_bk(bk) {}
   bool operator == (const SortItem & s) { return m_bk.internalElement() == s.m_bk.internalElement(); }
   bool isNull() const { return m_bk.isNull(); }
   SortItem previousSibling() const { return m_bk.parentGroup().previous(m_bk); }
   SortItem nextSibling() const { return m_bk.parentGroup().next(m_bk); }
   const KBookmark & bookmark() const { return m_bk; }
private:
   KBookmark m_bk;
};

class SortByName
{
public:
   static QString key( const SortItem &item )
   { return (item.bookmark().isGroup() ? "a" : "b") + item.bookmark().fullText().lower(); }
};

void SortCommand::execute()
{
   if ( m_commands.isEmpty() ) {
      KBookmarkGroup grp = KEBTopLevel::bookmarkManager()->findByAddress( m_groupAddress ).toGroup();
      Q_ASSERT( !grp.isNull() );
      SortItem firstChild( grp.first() );
      // This will call moveAfter, which will add the subcommands for moving the items
      kInsertionSort<SortItem, SortByName, QString, SortCommand> ( firstChild, *this );
   } else {
      // We've been here before
      KMacroCommand::execute();
   }
}

void SortCommand::moveAfter( const SortItem & moveMe, const SortItem & afterMe )
{
   /*
      kdDebug() << "SortCommand::moveAfter moveMe=" << moveMe.bookmark().address() << " " << moveMe.bookmark().fullText() << endl;
      if (!afterMe.isNull())
      kdDebug() << "                       afterMe=" << afterMe.bookmark().address() << " " << afterMe.bookmark().fullText() << endl;
      else
      kdDebug() << "                       afterMe=<null>" << endl;
    */
   QString destAddress = afterMe.isNull() ?
      KBookmark::parentAddress( moveMe.bookmark().address() ) + "/0" : // move as first child
      KBookmark::nextAddress(afterMe.bookmark().address()); // move after "afterMe"
   MoveCommand * cmd = new MoveCommand( QString::null, moveMe.bookmark().address(), destAddress );
   cmd->execute(); // do it now, the rest of the algorithm relies on it !
   addCommand( cmd );
}

void SortCommand::unexecute()
{
   KMacroCommand::unexecute();
}


/////////////

void ImportCommand::execute()
{
   KBookmarkGroup bkGroup;

   if ( !m_folder.isEmpty() ) {
      if (m_bookmarksType != BK_XBEL) {
         // Find or create "Netscape Bookmarks" toplevel item
         // Hmm, let's just create it. The user will clean up if he imports twice.
         bkGroup = KEBTopLevel::bookmarkManager()->root().createNewFolder(KEBTopLevel::bookmarkManager(),m_folder,false);
         bkGroup.internalElement().setAttribute("icon", m_icon);
         m_group = bkGroup.address();
      }

   } else {
      // Import into the root, after cleaning it up
      bkGroup = KEBTopLevel::bookmarkManager()->root();
      delete m_cleanUpCmd;
      m_cleanUpCmd = DeleteCommand::deleteAll( bkGroup );
      // Unselect current item, it doesn't exist anymore
      KEBTopLevel::self()->listView()->clearSelection();
      m_cleanUpCmd->execute();
      m_group = ""; // import at the root
   }

   if (m_bookmarksType == BK_XBEL) {
      xbelExecute();

   } else {
      mstack.push( &bkGroup );

      if (m_bookmarksType == BK_NS) {
         nsExecute();
      } else if (m_bookmarksType == BK_IE) {
         IEExecute();
      } else if (m_bookmarksType == BK_OPERA) {
         operaExecute();
      }

      // Save memory
      mlist.clear();
      mstack.clear();
   }

}

void ImportCommand::connectImporter( const QObject *importer )
{
   connect( importer,
         SIGNAL( newBookmark( const QString &, const QCString &, const QString & ) ), 
         SLOT( newBookmark( const QString &, const QCString &, const QString & ) ) );
   connect( importer,
         SIGNAL( newFolder( const QString &, bool, const QString & ) ),
         SLOT( newFolder( const QString &, bool, const QString & ) ) );
   connect( importer,
         SIGNAL( newSeparator() ),
         SLOT( newSeparator() ) );
   connect( importer,
         SIGNAL( endFolder() ),
         SLOT( endFolder() ) );
}

void ImportCommand::operaExecute()
{
   KOperaBookmarkImporter importer(m_fileName);
   connectImporter( &importer );
   importer.parseOperaBookmarks();
} 

void ImportCommand::IEExecute()
{
   KIEBookmarkImporter importer(m_fileName);
   connectImporter( &importer );
   importer.parseIEBookmarks();
} 

void ImportCommand::nsExecute()
{
   KNSBookmarkImporter importer(m_fileName);
   connectImporter( &importer );
   importer.parseNSBookmarks( m_utf8 );
}

void ImportCommand::xbelExecute()
{
   KBookmarkManager * pManager;
   pManager = KBookmarkManager::managerForFile( m_fileName, false );
   QDomDocument doc = KEBTopLevel::bookmarkManager()->internalDocument();

   // get the xbel
   QDomNode subDoc = pManager->internalDocument().namedItem("xbel").cloneNode();

   if ( !m_folder.isEmpty() ) {
      // transform into folder
      subDoc.toElement().setTagName("folder");

      // clear attributes
      QStringList tags;
      for (uint i = 0; i < subDoc.attributes().count(); i++) {
         tags << subDoc.attributes().item(i).toAttr().name();
      }
      for ( QStringList::Iterator it = tags.begin(); it != tags.end(); ++it ) {
         subDoc.attributes().removeNamedItem( (*it) );
      }

      subDoc.toElement().setAttribute("icon", m_icon);

      // give the folder a name
      QDomElement textElem = doc.createElement( "title" );
      subDoc.insertBefore(textElem, subDoc.firstChild());
      textElem.appendChild(doc.createTextNode( m_folder ));
   }

   // import and add it
   QDomNode node = doc.importNode( subDoc, true );

   if ( !m_folder.isEmpty() ) {
      KEBTopLevel::bookmarkManager()->root().internalElement().appendChild(node);
      m_group = KBookmarkGroup(node.toElement()).address();
   } else {
      QDomElement root = KEBTopLevel::bookmarkManager()->root().internalElement();

      QValueList<QDomElement> childList;

      QDomNode n = subDoc.firstChild().toElement();
      while( !n.isNull() ) {
         QDomElement e = n.toElement(); // try to convert the node to an element.
         if( !e.isNull() ) {
            childList.append( e );
         }
         n = n.nextSibling();
      }

      QValueList<QDomElement>::Iterator it = childList.begin();
      QValueList<QDomElement>::Iterator end = childList.end();
      for ( ; it!= end ; ++it ) {
         root.appendChild( *it );
      }
   }

}


void ImportCommand::unexecute()
{
   if ( !m_folder.isEmpty() ) {
      // We created a group -> just delete it
      DeleteCommand cmd(QString::null, m_group);
      cmd.execute();

   } else {
      // We imported at the root -> delete everything
      KBookmarkGroup root = KEBTopLevel::bookmarkManager()->root();
      KCommand * cmd = DeleteCommand::deleteAll( root );
      // Unselect current item, it doesn't exist anymore
      KEBTopLevel::self()->listView()->clearSelection();
      cmd->execute();
      delete cmd;
      // And recreate what was there before
      m_cleanUpCmd->unexecute();
   }
}

void ImportCommand::newBookmark( const QString & text, const QCString & url, const QString & additionnalInfo )
{
   KBookmark bk = mstack.top()->addBookmark( KEBTopLevel::bookmarkManager(), text, QString::fromUtf8(url), QString::null, false );
   // Store additionnal info
   bk.internalElement().setAttribute("netscapeinfo",additionnalInfo);
}

void ImportCommand::newFolder( const QString & text, bool open, const QString & additionnalInfo )
{
   // We use a qvaluelist so that we keep pointers to valid objects in the stack
   mlist.append( mstack.top()->createNewFolder( KEBTopLevel::bookmarkManager(), text,false ) );
   mstack.push( &(mlist.last()) );
   // Store additionnal info
   QDomElement element = mlist.last().internalElement();
   element.setAttribute("netscapeinfo",additionnalInfo);
   element.setAttribute("folded",open?"no":"yes");
}

void ImportCommand::newSeparator()
{
   mstack.top()->createNewSeparator();
}

void ImportCommand::endFolder()
{
   mstack.pop();
}

////////////////////////////////////////////////////////////////////////////
//
// TestLink
////////////////////////////////////////////////////////////////////////////

TestLink::TestLink(QValueList<KBookmark> bks) : m_bks(bks)
{
   connect( this, SIGNAL( deleteSelf(TestLink *)),
         KEBTopLevel::self(), SLOT(slotCancelTest(TestLink *)));
   m_job = 0;
   doNext();
}

TestLink::~TestLink()
{
   if (m_job) {
      kdDebug() << "JOB kill\n";
      KEBListViewItem *cur_item = KEBTopLevel::self()->findByAddress(m_book.address());
      cur_item->restoreStatus(m_oldStatus);
      m_job->disconnect();
      m_job->kill(false);
   }
}

void TestLink::doNext()
{
   kdDebug() << "TestLink::doNext" << endl;

   if (m_bks.count() == 0) {
      emit deleteSelf(this);
      return;
   }

   QValueListIterator<KBookmark> head = m_bks.begin();
   KBookmark bk = (*head);

   if (!bk.isGroup() && !bk.isSeparator() && bk.address() != "ERROR") {
      m_url = bk.url().url();

      kdDebug() << "TestLink::setCurrent " << m_url << " : " << bk.address() << "\n";

      m_job = KIO::get(bk.url(), true, false);
      connect(m_job, SIGNAL( result( KIO::Job *)),
            this, SLOT( jobResult(KIO::Job *)));
      connect(m_job, SIGNAL( data( KIO::Job *,  const QByteArray &)),
            this, SLOT( jobData(KIO::Job *, const QByteArray &)));
      m_job->addMetaData("errorPage", "true");

      KEBListViewItem *cur_item = KEBTopLevel::self()->findByAddress(bk.address());
      cur_item->setTmpStatus(i18n("Checking..."), m_oldStatus);

      m_book = bk;
      m_bks.remove(head);

   } else {
      m_bks.remove(head);
      doNext();
   }
}

void TestLink::setMod(KEBListViewItem *item, QString modDate) 
{
   time_t modt =  KRFCDate::parseDate(modDate);
   QString ms;
   ms.setNum(modt);
   item->nsPut(ms);
}

void TestLink::jobData(KIO::Job *job, const QByteArray &data)
{
   KEBListViewItem *cur_item = KEBTopLevel::self()->findByAddress(m_book.address());
   KIO::TransferJob *transfer = (KIO::TransferJob *)job;

   m_errSet = false;
   QString arrStr = data;

   if (transfer->isErrorPage()) {
      QStringList lines = QStringList::split('\n',arrStr);

      for ( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it ) {
         int open_pos = (*it).find("<title>", 0, false);
         if (open_pos >= 0) {
            QString leftover = (*it).mid(open_pos + 7);
            int close_pos = leftover.findRev("</title>", -1, false);
            if (close_pos >= 0) {
               // if no end tag found then just 
               // print the first line of the <title>
               leftover = leftover.left(close_pos);
            }
            cur_item->nsPut(KCharsets::resolveEntities(leftover));
            m_errSet = true;
            break;
         }
      }

   } else {
      QString modDate = transfer->queryMetaData("modified");
      if (!modDate.isEmpty()) {
         setMod(cur_item, modDate);
      }
   }

   transfer->kill(false);
}

void TestLink::jobResult(KIO::Job *job)
{
   m_job = 0;

   KEBListViewItem *cur_item = KEBTopLevel::self()->findByAddress(m_book.address());
   KIO::TransferJob *transfer = (KIO::TransferJob *)job;
   QString modDate = transfer->queryMetaData("modified");

   if (transfer->error()) {
      QString jerr = job->errorString();

      if (!jerr.isEmpty()) {
         jerr.replace( QRegExp("\n")," ");
         // can we assume that errorString will contain no entities?
         cur_item->nsPut(jerr);
      } else if (!modDate.isEmpty()) {
         setMod(cur_item, modDate);
      } else if (!m_errSet) {
         setMod(cur_item, "0");
      }

   } else if (!modDate.isEmpty()) {
      setMod(cur_item, modDate);
   } else if (!m_errSet) {
      setMod(cur_item, "0");
   }

   cur_item->modUpdate();

   doNext();
}


#include "commands.moc"
