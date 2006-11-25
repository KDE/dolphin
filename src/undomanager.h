/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#ifndef UNDOMANAGER_H
#define UNDOMANAGER_H

#include <qobject.h>
#include <q3valuelist.h>
#include <kurl.h>
#include <kio/jobclasses.h>

class ProgressIndicator;

/**
 * @short Represents a file manager command which can be undone and redone.
 *
 * A command is specified by a type, a list of source Urls and a
 * destination Url.
 *
 * Due to the fixed set of commands a file manager offers this class is
 * a very simplified version of the classic command pattern.
 *
 * @see UndoManager
 * @author Peter Penz <peter.penz@gmx.at>
 */
class DolphinCommand
{
public:
    enum Type {
        Copy,
        Move,
        Link,
        Rename,
        Trash,
        CreateFolder,
        CreateFile
    };

    DolphinCommand();
    DolphinCommand(Type type, const KUrl::List& source, const KUrl& dest);
    ~DolphinCommand(); // non-virtual

    DolphinCommand& operator = (const DolphinCommand& command);
    Type type() const { return m_type; }
    void setSource(const KUrl::List source) { m_source = source; }
    const KUrl::List& source() const { return m_source; }
    const KUrl& destination() const { return m_dest; }

private:
    Type m_type;
    int m_macroIndex;
    KUrl::List m_source;
    KUrl m_dest;

    friend class UndoManager;   // allow to modify m_macroIndex
};

/**
 * @short Stores all file manager commands which can be undone and redone.
 *
 * During the undo and redo operations a progress information is
 * shown in the status bar.
 *
 *	@author Peter Penz <peter.penz@gmx.at>
 */
class UndoManager : public QObject   // TODO switch to KonqUndoManager (multi-process, async, more robust on complex operations, no redo though)
{
    Q_OBJECT

public:
    static UndoManager& instance();

    /**
     * Adds the command \a command to the undo list. The command
     * can be undone by invoking UndoManager::undo().
     */
    void addCommand(const DolphinCommand& command);

    /**
     * Allows to summarize several commands into one macro, which
     * can be undo in one stop by UndoManager::undo(). Example
     * \code
     * UndoManager& undoMan = UndoManager.instance();
     * undoMan.beginMacro();
     * undoMan.addCommand(...);
     * undoMan.addCommand(...);
     * undoMan.addCommand(...);
     * undoMan.endMacro();
     * \endcode
     * It is not allowed to do nested macro recordings.
     */
    void beginMacro();

    /**
     * Marks the end of a macro command. See UndoManager::beginMacro()
     * for sample code.
     */
    void endMacro();

public slots:
    /**
     * Performs an undo operation on the last command which has
     * been added by UndoManager::addCommand().
     */
    void undo();

    /**
     * Performs a redo operation on the last command where an undo
     * operation has been applied.
     */
    void redo();

signals:
    /**
     * Is emitted if whenever the availability state
     * of the current undo operation changes.
     */
    void undoAvailable(bool available);

    /**
     * Is emitted whenever the text of the current
     * undo operation changes
     * (e. g. from 'Undo: Delete' to 'Undo: Copy')
     */
    void undoTextChanged(const QString& text);

    /**
     * Is emitted if whenever the availability state
     * of the current redo operation changes.
     */
    void redoAvailable(bool available);

    /**
     * Is emitted whenever the text of the current
     * redo operation changes
     * (e. g. from 'Redo: Delete' to 'Redo: Copy')
     */
    void redoTextChanged(const QString& text);

protected:
    UndoManager();
    virtual ~UndoManager();
    QString commandText(const DolphinCommand& command) const;

private slots:
    /**
     * Slot for the percent information of the I/O slaves.
     * Delegates the updating of the progress information
     * to UndoManager::updateProgress().
     */
    void slotPercent(KJob* job, unsigned long percent);

    /**
     * Updates the progress information of the statusbar
     * by accessing the progress indicator information.
     */
    void updateProgress();

private:
    bool m_recordMacro;
    int m_historyIndex;
    int m_macroCounter;
    Q3ValueList<DolphinCommand> m_history;
    ProgressIndicator* m_progressIndicator;

    /**
     * Dependent from the current history index \a m_historyIndex
     * the number of macro commands is written to the output
     * parameter \a macroCount. The number of steps for all macro
     * commands is written to the output parameter \a progressCount.
     *
     * Per default \a macroCount is 1 and \a progressCount represents
     * the number of operations for one command.
     */
    void calcStepsCount(int& macroCount,
                        int& progressCount);
};

#endif
