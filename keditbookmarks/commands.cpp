/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

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
#include "toplevel.h"
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>
#include <kdebug.h>
#include <klistview.h>
#include <klocale.h>
#include "kinsertionsort.h"

void MoveCommand::execute()
{
    kdDebug() << "MoveCommand::execute moving from=" << m_from << " to=" << m_to << endl;

    // Look for m_from in the QDom tree
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_from );
    Q_ASSERT( !bk.isNull() );
    //kdDebug() << "BEFORE:" << KBookmarkManager::self()->internalDocument().toString() << endl;
    int posInOldParent = KBookmark::positionInParent( m_from );
    KBookmark oldParent = KBookmarkManager::self()->findByAddress( KBookmark::parentAddress( m_from ) );
    KBookmark oldPreviousSibling = posInOldParent == 0 ? KBookmark(QDomElement())
                                   : KBookmarkManager::self()->findByAddress( KBookmark::previousAddress( m_from ) );

    // Look for m_to in the QDom tree (as parent address and position in parent)
    int posInNewParent = KBookmark::positionInParent( m_to );
    QString parentAddress = KBookmark::parentAddress( m_to );
    //kdDebug() << "MoveCommand::execute parentAddress=" << parentAddress << " posInNewParent=" << posInNewParent << endl;
    KBookmark newParentBk = KBookmarkManager::self()->findByAddress( parentAddress );
    Q_ASSERT( !newParentBk.isNull() );
    Q_ASSERT( newParentBk.isGroup() );

    if ( posInNewParent == 0 ) // First child
    {
        newParentBk.toGroup().moveItem( bk, QDomElement() );
    }
    else
    {
        QString afterAddress = KBookmark::previousAddress( m_to );
        kdDebug() << "MoveCommand::execute afterAddress=" << afterAddress << endl;
        KBookmark afterNow = KBookmarkManager::self()->findByAddress( afterAddress );
        Q_ASSERT(!afterNow.isNull());
        bool result = newParentBk.toGroup().moveItem( bk, afterNow );
        Q_ASSERT(result);
        kdDebug() << "MoveCommand::execute after moving in the dom tree : item=" << bk.address() << endl;
    }

    // Ok, now this is the most tricky bit.
    // Because we moved stuff around, the addresses from/to can have changed
    // So we look into the dom tree to get the new positions, using a reference
    // The reference is :
    if ( posInOldParent == 0 ) // the old parent, if we were the first child
        m_from = oldParent.address() + "/0";
    else // otherwise the previous sibling
        m_from = KBookmark::nextAddress( oldPreviousSibling.address() );
    m_to = bk.address();
    kdDebug() << "MoveCommand::execute : new addresses from=" << m_from << " to=" << m_to << endl;
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
    // Gather some info
    QString parentAddress = KBookmark::parentAddress( m_to );
    KBookmarkGroup parentGroup = KBookmarkManager::self()->findByAddress( parentAddress ).toGroup();
    QString previousSibling = KBookmark::previousAddress( m_to );
    //kdDebug() << "CreateCommand::execute previousSibling=" << previousSibling << endl;
    KBookmark prev = previousSibling.isEmpty() ? KBookmark(QDomElement())
                     : KBookmarkManager::self()->findByAddress( previousSibling );

    // Create
    KBookmark bk = KBookmark(QDomElement());
    if (m_originalBookmark.isNull())
        if (m_separator)
            bk = parentGroup.createNewSeparator();
        else
            if (m_group)
            {
                Q_ASSERT( !m_text.isEmpty() );
                bk = parentGroup.createNewFolder( m_text );

                kdDebug() << "CreateCommand::execute " << m_group << " open : " << m_open << endl;
                bk.internalElement().setAttribute( "folded", m_open ? "no" : "yes" );
                if(!m_iconPath.isEmpty())
                    bk.internalElement().setAttribute( "icon",m_iconPath );
            }
            else
                bk = parentGroup.addBookmark( m_text, m_url, m_iconPath);
    else
        bk = m_originalBookmark;

    // Move to right position
    parentGroup.moveItem( bk, prev );
    if ( !name().isEmpty() )
    {
        kdDebug() << "Opening parent" << endl;
        // Open the parent (useful if it was empty) - only for manual commands
        parentGroup.internalElement().setAttribute( "folded", "no" );
    }
    Q_ASSERT( bk.address() == m_to );
}

void CreateCommand::unexecute()
{
    kdDebug() << "CreateCommand::unexecute deleting " << m_to << endl;
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_to );
    Q_ASSERT( !bk.isNull() );
    Q_ASSERT( !bk.parentGroup().isNull() );
    // Update GUI
    // but first we need to select a valid item
    KListView * lv = KEBTopLevel::self()->listView();
    KEBListViewItem * item = static_cast<KEBListViewItem*>(lv->selectedItem());
    if (item && item->bookmark().address() == m_to)
    {
        lv->setSelected(item,false);
        // can't use itemBelow here, in case we're deleting a folder
        if ( item->nextSibling() )
            lv->setSelected( item->nextSibling(), true );
        else // No next sibling ? Go to previous one, then.
        {
            QString prevAddr = bk.parentGroup().previousAddress( bk.address() );
            if ( !prevAddr.isEmpty() )
            {
                QListViewItem * prev = KEBTopLevel::self()->findByAddress( prevAddr );
                if (prev)
                    lv->setSelected( prev, true );
            }
            else // No previous sibling either ? Go up, then.
            {
                QListViewItem * par = KEBTopLevel::self()->findByAddress( KBookmark::parentAddress( bk.address() ) );
                if (par)
                    lv->setSelected( par, true );
            }
        }
    }

    bk.parentGroup().deleteBookmark( bk );
}

void DeleteCommand::execute()
{
    kdDebug() << "DeleteCommand::execute " << m_from << endl;
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_from );
    Q_ASSERT(!bk.isNull());
    if ( !m_cmd )
        if ( bk.isGroup() )
        {
            m_cmd = new CreateCommand(QString::null, m_from, bk.fullText(),bk.icon(),
                                      bk.internalElement().attribute("folded")=="no");
            m_subCmd = deleteAll( bk.toGroup() );
            m_subCmd->execute();
        }
        else
            if ( bk.isSeparator() )
                m_cmd = new CreateCommand(QString::null, m_from );
            else
            {
                m_cmd = new CreateCommand(QString::null, m_from, bk.fullText(),bk.icon(),bk.url());
            }

    m_cmd->unexecute();
}

void DeleteCommand::unexecute()
{
    m_cmd->execute();
    if (m_subCmd)
        m_subCmd->unexecute();
}

KMacroCommand * DeleteCommand::deleteAll( const KBookmarkGroup & parentGroup )
{
    KMacroCommand * cmd = new KMacroCommand( QString::null );
    QStringList lstToDelete;
    // We need to delete from the end, to avoid index shifting
    for ( KBookmark bk = parentGroup.first() ; !bk.isNull() ; bk = parentGroup.next(bk) )
        lstToDelete.prepend( bk.address() );
    for ( QStringList::Iterator it = lstToDelete.begin(); it != lstToDelete.end() ; ++it )
    {
        kdDebug() << "DeleteCommand::deleteAll: deleting " << *it << endl;
        cmd->addCommand( new DeleteCommand( QString::null, *it ) );
    }
    return cmd;
}

void EditCommand::execute()
{
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_address );
    Q_ASSERT( !bk.isNull() );
    m_reverseEditions.clear();
    QValueList<Edition>::Iterator it = m_editions.begin();
    for ( ; it != m_editions.end() ; ++it )
    {
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
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_address );
    Q_ASSERT( !bk.isNull() );

    QDomText domtext = bk.internalElement().elementsByTagName("title").item(0).firstChild().toText();

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
    { return item.bookmark().fullText().lower(); }
};

void SortCommand::execute()
{
    if ( m_commands.isEmpty() )
    {
        KBookmarkGroup grp = KBookmarkManager::self()->findByAddress( m_groupAddress ).toGroup();
        Q_ASSERT( !grp.isNull() );
        SortItem firstChild( grp.first() );
        // This will call moveAfter, which will add the subcommands for moving the items
        kInsertionSort<SortItem, SortByName, QString, SortCommand> ( firstChild, *this );
    } else
        // We've been here before
        KMacroCommand::execute();
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
    KBookmarkGroup netscapeGroup;
    if ( !m_folder.isEmpty() )
    {
        // Find or create "Netscape Bookmarks" toplevel item
        // Hmm, let's just create it. The user will clean up if he imports twice.
        netscapeGroup = KBookmarkManager::self()->root().createNewFolder(m_folder);
        netscapeGroup.internalElement().setAttribute("icon", m_icon);
        m_group = netscapeGroup.address();
    } else
    {
        // Import into the root, after cleaning it up
        netscapeGroup = KBookmarkManager::self()->root();
        delete m_cleanUpCmd;
        m_cleanUpCmd = DeleteCommand::deleteAll( netscapeGroup );
        // Unselect current item, it doesn't exist anymore
        KEBTopLevel::self()->listView()->clearSelection();
        m_cleanUpCmd->execute();
        m_group = ""; // import at the root
    }

    mstack.push( &netscapeGroup );
    KNSBookmarkImporter importer(m_fileName);
    connect( &importer, SIGNAL( newBookmark( const QString &, const QCString &, const QString & ) ),
             SLOT( newBookmark( const QString &, const QCString &, const QString & ) ) );
    connect( &importer, SIGNAL( newFolder( const QString &, bool, const QString & ) ),
             SLOT( newFolder( const QString &, bool, const QString & ) ) );
    connect( &importer, SIGNAL( newSeparator() ), SLOT( newSeparator() ) );
    connect( &importer, SIGNAL( endMenu() ), SLOT( endMenu() ) );
    importer.parseNSBookmarks( m_utf8 );
    // Save memory
    mlist.clear();
    mstack.clear();
}

void ImportCommand::unexecute()
{
    if ( !m_folder.isEmpty() )
    {
        // We created a group -> just delete it
        DeleteCommand cmd(QString::null, m_group);
        cmd.execute();
    }
    else
    {
        // We imported at the root -> delete everything
        KBookmarkGroup root = KBookmarkManager::self()->root();
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
    KBookmark bk = mstack.top()->addBookmark( text, QString::fromUtf8(url) );
    // Store additionnal info
    bk.internalElement().setAttribute("netscapeinfo",additionnalInfo);
}

void ImportCommand::newFolder( const QString & text, bool open, const QString & additionnalInfo )
{
    // We use a qvaluelist so that we keep pointers to valid objects in the stack
    mlist.append( mstack.top()->createNewFolder( text ) );
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

void ImportCommand::endMenu()
{
    mstack.pop();
}

#include "commands.moc"
