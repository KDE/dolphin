/* This file is part of the KDE project
   Copyright (C) 2000 Werner Trobin <trobin@kde.org>

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

#ifndef gcommand_h
#define gcommand_h

#include <qlist.h>
#include <qstring.h>
#include <qobject.h>

class KAction;
class KActionCollection;

// gcommand will hopefully go in kdelibs one day, with a k instead of the g :)
#define GCommand KCommand
#define GCommandHistory KCommandHistory

/**
 * The abstract base class for all Commands. Commands are used to
 * store information needed for Undo/Redo functionality...
 */
class GCommand
{
protected:
    GCommand(const QString &name) : m_name(name) {}

public:
    virtual ~GCommand() {}

    // Note: Check for object!=0L !!!
    virtual void execute() = 0;
    virtual void unexecute() = 0;

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name=name; }

private:
    QString m_name;
};


/**
 * The command history stores a (user) configurable amount of
 * Commands. It keeps track of its size and deletes commands
 * if it gets too large. The user can set a maximum undo and
 * a maximum redo limit (e.g. max. 50 undo / 30 redo commands).
 * The GCommandHistory keeps track of the "borders" and deletes
 * commands, if appropriate. It also activates/deactivates the
 * undo/redo actions in the menu and changes the text according
 * to the name of the command.
 */
class GCommandHistory : public QObject {
    Q_OBJECT
public:
    /**
     * Create a command history. This also creates an
     * undo and a redo action, in the @p actionCollection.
     */
    GCommandHistory(KActionCollection *actionCollection);

    ~GCommandHistory();

    void clear();

    void addCommand(GCommand *command);

    const int &undoLimit() { return m_undoLimit; }
    void setUndoLimit(const int &limit);
    const int &redoLimit() { return m_redoLimit; }
    void setRedoLimit(const int &limit);

public slots:
    void undo();
    void redo();

private:
    void clipCommands();  // ensures that the limits are kept
    void helpRedo();

    QList<GCommand> m_commands;
    GCommand *m_present;
    KAction *m_undo, *m_redo;
    int m_undoLimit, m_redoLimit;
    bool m_first;  // attention: it's the first command in the list!
};

#endif // gcommand_h
