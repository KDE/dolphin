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

#include <kcommand.h>
#include <kbookmark.h>

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
          m_group(false), m_separator(true), m_originalBookmark(QDomElement())
    {}

    // Create a bookmark
    CreateCommand( const QString & name, const QString & address,
                   const QString & text, const QString &iconPath, const KURL & url )
        : KCommand(name), m_to(address), m_text(text),m_iconPath(iconPath), m_url(url),
          m_group(false), m_separator(false), m_originalBookmark(QDomElement())
    {}

    // Create a folder
    CreateCommand( const QString & name, const QString & address,
                   const QString & text, const QString &iconPath, bool open )
        : KCommand(name), m_to(address), m_text(text),m_iconPath(iconPath),
          m_group(true), m_separator(false), m_open(open), m_originalBookmark(QDomElement())
    {}

    // Create a copy of an existing bookmark (whatever it is)
    CreateCommand( const QString & name, const QString & address,
                   const KBookmark & original )
        : KCommand(name), m_to(address), m_group(false), m_separator(false),
          m_open(false), m_originalBookmark( original )
    {}

    virtual ~CreateCommand() {}
    virtual void execute();
    virtual void unexecute();
private:
    QString m_to;
    QString m_text;
    QString m_iconPath;
    KURL m_url;
    bool m_group:1;
    bool m_separator:1;
    bool m_open:1;
    KBookmark m_originalBookmark;
};

class DeleteCommand : public KCommand
{
public:
    DeleteCommand( const QString & name, const QString & from )
        : KCommand(name), m_from(from), m_cmd(0L), m_subCmd(0L)
    {}
    virtual ~DeleteCommand()
    { delete m_cmd; }
    virtual void execute();
    virtual void unexecute();

    static KMacroCommand * deleteAll( const KBookmarkGroup & parentGroup );
private:
    QString m_from;
    KCommand * m_cmd;
    KMacroCommand * m_subCmd;
};

class EditCommand : public KCommand
{
public:

    struct Edition {
        Edition() {} // For QValueList
        Edition(const QString & a, const QString & v) : attr(a), value(v) {}
        QString attr;
        QString value;
    };

    /**
     * This command changes the value of one attribute of the bookmark @p address
     */
    EditCommand( const QString & name, const QString & address,
                 Edition edition ) :
        KCommand(name), m_address(address)
    {
        m_editions.append(edition);
    }

    /**
     * This command changes the value of several attributes of the bookmark @p address
     */
    EditCommand( const QString & name, const QString & address,
                 const QValueList<Edition> & editions ) :
        KCommand(name), m_address(address), m_editions(editions)
    {}
    virtual ~EditCommand() {}
    virtual void execute();
    virtual void unexecute();
private:
    QString m_address;
    QValueList<Edition> m_editions;
    QValueList<Edition> m_reverseEditions;
};

class RenameCommand : public KCommand
{
public:
    RenameCommand( const QString & name, const QString & address, const QString & newText )
        : KCommand(name), m_address(address), m_newText(newText) {}
    virtual ~RenameCommand() {}
    virtual void execute();
    virtual void unexecute();
private:
    QString m_address;
    QString m_newText;
    QString m_oldText;
};

class SortItem;

class SortCommand : public KMacroCommand
{
public:
    SortCommand( const QString & name, const QString &groupAddress /*TODO criteria.. enum ?*/ )
        : KMacroCommand( name ), m_groupAddress( groupAddress ) {}
    virtual ~SortCommand() {}
    virtual void execute();
    virtual void unexecute();
    // internal
    void moveAfter( const SortItem & moveMe, const SortItem & afterMe );
private:
    QString m_groupAddress;
};

#include <qstack.h>
#include <qobject.h>
class ImportCommand : public QObject, public KCommand
{
    Q_OBJECT
public:
    /**
     * @param name name of the command
     * @param fileName HTML file to import
     * @param folder name of the folder to create. Empty for no creation (root()).
     * @param icon icon for the new folder, if @p folder isn't empty
     * @param utf8 true if the HTML is in utf-8 encoding
     */
    ImportCommand( const QString & name, const QString & fileName, const QString & folder, const QString & icon, bool utf8 )
        : KCommand(name), m_fileName(fileName), m_folder(folder), m_icon(icon), m_cleanUpCmd(0L), m_utf8(utf8)
    {}
    virtual ~ImportCommand() {}
    virtual void execute();
    virtual void unexecute();

protected slots:
    void newBookmark( const QString & text, const QCString & url, const QString & additionnalInfo );
    void newFolder( const QString & text, bool open, const QString & additionnalInfo );
    void newSeparator();
    void endMenu();

private:
    QStack<KBookmarkGroup> mstack;
    QValueList<KBookmarkGroup> mlist;
    QString m_fileName;
    QString m_folder;
    QString m_icon;
    QString m_group;
    KMacroCommand * m_cleanUpCmd;
    bool m_utf8;
};

#endif
