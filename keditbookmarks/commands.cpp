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
#include <kdebug.h>
#include <klistview.h>

void MoveCommand::execute()
{
    kdDebug() << "MoveCommand::execute moving from=" << m_from << " to=" << m_to << endl;

    // Look for m_from in the QDom tree
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_from );
    ASSERT( !bk.isNull() );
    //kdDebug() << "BEFORE:" << KBookmarkManager::self()->internalDocument().toCString() << endl;
    int posInOldParent = KBookmark::positionInParent( m_from );
    KBookmark oldParent = KBookmarkManager::self()->findByAddress( KBookmark::parentAddress( m_from ) );
    KBookmark oldPreviousSibling = posInOldParent == 0 ? KBookmark(QDomElement())
                                   : KBookmarkManager::self()->findByAddress( KBookmark::previousAddress( m_from ) );

    // Look for m_to in the QDom tree (as parent address and position in parent)
    int posInNewParent = KBookmark::positionInParent( m_to );
    QString parentAddress = KBookmark::parentAddress( m_to );
    kdDebug() << "MoveCommand::execute parentAddress=" << parentAddress << " posInNewParent=" << posInNewParent << endl;
    KBookmark newParentBk = KBookmarkManager::self()->findByAddress( parentAddress );
    ASSERT( !newParentBk.isNull() );

    if ( posInNewParent == 0 ) // First child
    {
        newParentBk.internalElement().insertBefore( bk.internalElement(), QDomNode() );
    }
    else
    {
        QString afterAddress = KBookmark::previousAddress( m_to );
        kdDebug() << "MoveCommand::execute afterAddress=" << afterAddress << endl;
        KBookmark afterNow = KBookmarkManager::self()->findByAddress( afterAddress );
        ASSERT(!afterNow.isNull());
        QDomNode result = newParentBk.internalElement().insertAfter( bk.internalElement(), afterNow.internalElement() );
        ASSERT(!result.isNull());
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

    //kdDebug() << "AFTER:" << KBookmarkManager::self()->internalDocument().toCString() << endl;

    // Update GUI
    KEBTopLevel::self()->update();
}

void MoveCommand::unexecute()
{
    // Let's not duplicate code.
    MoveCommand undoCmd( "dummy", m_to, m_from );
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
    if (m_separator)
        bk = parentGroup.createNewSeparator();
    else
        if (m_group)
        {
            bk = parentGroup.createNewFolder( m_text );
            m_text = bk.fullText(); // remember it, we won't have to ask it again
        }
        else
            bk = parentGroup.addBookmark( m_text, m_url );

    // Move to right position
    parentGroup.moveItem( bk, prev );
    // Open the parent (useful if it was empty)
    parentGroup.internalElement().setAttribute( "OPEN", 1 );
    ASSERT( bk.address() == m_to );

    // Update GUI
    KEBTopLevel::self()->update();
}

void CreateCommand::unexecute()
{
    kdDebug() << "CreateCommand::unexecute deleting " << m_to << endl;
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_to );
    ASSERT( !bk.isNull() );
    // Update GUI
    // but first we need to select a valid item
    KListView * lv = KEBTopLevel::self()->listView();
    KEBListViewItem * item = static_cast<KEBListViewItem*>(lv->selectedItem());
    if (item && item->bookmark().address() == m_to)
    {
        lv->setSelected(item,false);
        // can't use itemBelow here, in case we're deleting a folder
        if ( item->nextSibling() )
            lv->setSelected(item->nextSibling(),true);
    }

    bk.parentGroup().deleteBookmark( bk );

    KEBTopLevel::self()->update();
}

void DeleteCommand::execute()
{
    kdDebug() << "DeleteCommand::execute " << m_from << endl;
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_from );
    if ( !m_cmd )
        if ( bk.isGroup() )
            m_cmd = new CreateCommand("dummy", m_from, bk.fullText());
        else
            if ( bk.isSeparator() )
                m_cmd = new CreateCommand("dummy", m_from );
            else
                m_cmd = new CreateCommand("dummy", m_from, bk.fullText(), bk.url());

    m_cmd->unexecute();
}

void DeleteCommand::unexecute()
{
    m_cmd->execute();
}
