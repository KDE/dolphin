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

#ifndef __commands_h
#define __commands_h

#include <gcommand.h>

// TODO MacroCommand

class MoveCommand : public KCommand
{
public:
    /**
     * This command stores the movement of an item in the tree.
     * Create it with itemsAlreadyMoved=true since KListView moves the
     * item before telling us about it.
     */
    MoveCommand( const QString & name, const QString & from, const QString & to )
        : KCommand(name), m_from(from), m_to(to)
    {}
    virtual ~MoveCommand() {}
    virtual void execute();
    virtual void unexecute();
private:
    QString m_from;
    QString m_to;
};

class CreateCommand : public KCommand
{
public:
    // Create a separator
    CreateCommand( const QString & name, const QString & address )
        : KCommand(name), m_to(address),
          m_group(false), m_separator(true)
    {}

    // Create a bookmark
    CreateCommand( const QString & name, const QString & address,
                   const QString & text, const QString & url )
        : KCommand(name), m_to(address), m_text(text), m_url(url),
          m_group(false), m_separator(false)
    {}

    // Create a folder
    CreateCommand( const QString & name, const QString & address,
                   const QString & text )
        : KCommand(name), m_to(address), m_text(text),
          m_group(true), m_separator(false)
    {}

    virtual ~CreateCommand() {}
    virtual void execute();
    virtual void unexecute();
private:
    QString m_to;
    QString m_text;
    QString m_url;
    bool m_group;
    bool m_separator;
};

class DeleteCommand : public KCommand
{
public:
    DeleteCommand( const QString & name, const QString & from )
        : KCommand(name), m_from(from), m_cmd(0L)
    {}
    virtual ~DeleteCommand()
    { delete m_cmd; }
    virtual void execute();
    virtual void unexecute();
private:
    QString m_from;
    KCommand * m_cmd;
};

#include <qstack.h>
#include <qobject.h>
#include <kbookmark.h>
class ImportCommand : public QObject, public KCommand
{
    Q_OBJECT
public:
    ImportCommand( const QString & name, const QString & fileName )
        : KCommand(name), m_fileName(fileName)
    {}
    virtual ~ImportCommand() {}
    virtual void execute();
    virtual void unexecute();

protected slots:
    void newBookmark( const QString & text, const QCString & url );
    void newFolder( const QString & text );
    void newSeparator();
    void endMenu();

private:
    QStack<KBookmarkGroup> mstack;
    QValueList<KBookmarkGroup> mlist;
    QString m_fileName;
    QString m_group;
};

#endif
