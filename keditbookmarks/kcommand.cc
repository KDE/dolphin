/* This file is part of the KDE project
   Copyright (C) 2000 Werner Trobin <trobin@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kcommand.h"
#include <kaction.h>
#include <kstdaction.h>
#include <kdebug.h>
#include <klocale.h>

KMacroCommand::KMacroCommand( const QString & name ) : KCommand(name)
{
    m_commands.setAutoDelete(true);
}

void KMacroCommand::addCommand(KCommand *command)
{
    m_commands.append(command);
}

void KMacroCommand::execute()
{
    QListIterator<KCommand> it(m_commands);
    for ( ; it.current() ; ++it )
        it.current()->execute();
}

void KMacroCommand::unexecute()
{
    QListIterator<KCommand> it(m_commands);
    it.toLast();
    for ( ; it.current() ; --it )
        it.current()->unexecute();
}


////////////

KCommandHistory::KCommandHistory(KActionCollection * actionCollection) :
    m_present(0L), m_undoLimit(50), m_redoLimit(30), m_first(false)
{
    m_undo = KStdAction::undo( this, SLOT( undo() ), actionCollection );
    m_redo = KStdAction::redo( this, SLOT( redo() ), actionCollection );

    m_commands.setAutoDelete(true);
    clear();
}

KCommandHistory::~KCommandHistory() {
}

void KCommandHistory::clear() {
    m_undo->setEnabled(false);
    m_undo->setText(i18n("No Undo Possible"));
    m_redo->setEnabled(false);
    m_redo->setText(i18n("No Redo Possible"));
    m_present = 0L;
}

void KCommandHistory::addCommand(KCommand *command, bool execute) {

    if(command==0L)
        return;

    int index;
    if(m_present!=0L && (index=m_commands.findRef(m_present))!=-1) {
        m_commands.insert(index+1, command);
        // truncate history
        while (m_commands.next())
            m_commands.remove();
        m_present=command;
        m_first=false;
        m_undo->setEnabled(true);
        m_undo->setText(i18n("Und&o: %1").arg(m_present->name()));
        /* can't happen anymore
        if(m_commands.next()!=0) {
            KCommand *tmp=m_commands.current();
            m_redo->setEnabled(true);
            m_redo->setText(i18n("Re&do: %1").arg(tmp->name()));
        }
        else */ {
            if(m_redo->isEnabled()) {
                m_redo->setEnabled(false);
                m_redo->setText(i18n("No Redo Possible"));
            }
        }
        clipCommands();
    }
    else { // either this is the first time we add a Command or something has gone wrong
        kdDebug() << "Initializing the Command History" << endl;
        m_commands.clear();
        m_commands.append(command);
        m_present=command;
        m_undo->setEnabled(true);
        m_undo->setText(i18n("Und&o: %1").arg(m_present->name()));
        m_redo->setEnabled(false);
        m_redo->setText(i18n("No Redo Possible"));
        m_first=true;
    }
    if ( execute )
    {
        command->execute();
        emit commandExecuted();
    }
}

void KCommandHistory::undo() {

    m_present->unexecute();
    m_redo->setEnabled(true);
    m_redo->setText(i18n("Re&do: %1").arg(m_present->name()));
    if(m_commands.findRef(m_present)!=-1 && m_commands.prev()!=0) {
        m_present=m_commands.current();
        m_undo->setEnabled(true);
        m_undo->setText(i18n("Und&o: %1").arg(m_present->name()));
    }
    else {
        m_undo->setEnabled(false);
        m_undo->setText(i18n("No Undo Possible"));
        m_first=true;
    }
    emit commandExecuted();
}

void KCommandHistory::redo() {

    if(m_first) {
        m_present->execute();
        m_first=false;
        m_commands.first();
    }
    else if(m_commands.findRef(m_present)!=-1 && m_commands.next()!=0) {
        m_present=m_commands.current();
        m_present->execute();
    }

    m_undo->setEnabled(true);
    m_undo->setText(i18n("Und&o: %1").arg(m_present->name()));

    if(m_commands.next()!=0) {
        KCommand *tmp=m_commands.current();
        m_redo->setEnabled(true);
        m_redo->setText(i18n("Re&do: %1").arg(tmp->name()));
    }
    else {
        if(m_redo->isEnabled()) {
            m_redo->setEnabled(false);
            m_redo->setText(i18n("No Redo Possible"));
        }
    }

    emit commandExecuted();
}

void KCommandHistory::setUndoLimit(const int &limit) {

    if(limit>0 && limit!=m_undoLimit) {
        m_undoLimit=limit;
        clipCommands();
    }
}

void KCommandHistory::setRedoLimit(const int &limit) {

    if(limit>0 && limit!=m_redoLimit) {
        m_redoLimit=limit;
        clipCommands();
    }
}

void KCommandHistory::clipCommands() {

    int count=m_commands.count();
    if(count<m_undoLimit && count<m_redoLimit)
        return;

    int index=m_commands.findRef(m_present);
    if(index>=m_undoLimit) {
        for(int i=0; i<=(index-m_undoLimit); ++i)
            m_commands.removeFirst();
        index=m_commands.findRef(m_present); // calculate the new
        count=m_commands.count();            // values (for the redo-branch :)
    }
    if((index+m_redoLimit)<count) {
        for(int i=0; i<(count-(index+m_redoLimit)); ++i)
            m_commands.removeLast();
    }
}

#include "kcommand.moc"
