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

#include "undomanager.h"
#include <klocale.h>
#include <kio/netaccess.h>
#include <qtimer.h>
#include <assert.h>

#include "dolphin.h"
#include "dolphinstatusbar.h"
#include "progressindicator.h"

DolphinCommand::DolphinCommand() :
    m_type(Copy),
    m_macroIndex(-1)
{
    // Implementation note: DolphinCommands are stored in a QValueList, whereas
    // QValueList requires a default constructor of the added class.
    // Instead of expressing this implementation detail to the interface by adding a
    // Type::Undefined just Type::Copy is used to assure that all members have
    // a defined state.
}

DolphinCommand::DolphinCommand(Type type,
                              const KUrl::List& source,
                              const KUrl& dest) :
    m_type(type),
    m_macroIndex(-1),
    m_source(source),
    m_dest(dest)
{
}

DolphinCommand::~DolphinCommand()
{
}

DolphinCommand& DolphinCommand::operator = (const DolphinCommand& command)
{
    m_type = command.m_type;
    m_source = command.m_source;
    m_dest = command.m_dest;
    return *this;
}

UndoManager& UndoManager::instance()
{
    static UndoManager* instance = 0;
    if (instance == 0) {
        instance = new UndoManager();
    }
    return *instance;
}

void UndoManager::addCommand(const DolphinCommand& command)
{
    ++m_historyIndex;

    if (m_recordMacro) {
        DolphinCommand macroCommand = command;
        macroCommand.m_macroIndex = m_macroCounter;
        m_history.insert(m_history.at(m_historyIndex), macroCommand);
    }
    else {
        m_history.insert(m_history.at(m_historyIndex), command);
    }

    emit undoAvailable(true);
    emit undoTextChanged(i18n("Undo: %1").arg(commandText(command)));

    // prevent an endless growing of the Undo history
    if (m_historyIndex > 10000) {
        m_history.erase(m_history.begin());
        --m_historyIndex;
    }
}

void UndoManager::beginMacro()
{
    assert(!m_recordMacro);
    m_recordMacro = true;
    ++m_macroCounter;
}

void UndoManager::endMacro()
{
    assert(m_recordMacro);
    m_recordMacro = false;
}

void UndoManager::undo()
{
    if (m_recordMacro) {
        endMacro();
    }

    if (m_historyIndex < 0) {
        return;
    }

    int progressCount = 0;
    int macroCount = 1;
    calcStepsCount(macroCount, progressCount);

    m_progressIndicator = new ProgressIndicator(i18n("Executing undo operation..."),
                                                i18n("Executed undo operation."),
                                                progressCount);

    for (int i = 0; i < macroCount; ++i) {
        const DolphinCommand command = m_history[m_historyIndex];
        --m_historyIndex;
        if (m_historyIndex < 0) {
            emit undoAvailable(false);
            emit undoTextChanged(i18n("Undo"));
        }
        else {
            emit undoTextChanged(i18n("Undo: %1").arg(commandText(m_history[m_historyIndex])));
        }

        if (m_historyIndex < static_cast<int>(m_history.count()) - 1) {
            emit redoAvailable(true);
            emit redoTextChanged(i18n("Redo: %1").arg(commandText(command)));
        }
        else {
            emit redoAvailable(false);
            emit redoTextChanged(i18n("Redo"));
        }

        KUrl::List sourceURLs = command.source();
        KUrl::List::Iterator it = sourceURLs.begin();
        const KUrl::List::Iterator end = sourceURLs.end();
        const QString destURL(command.destination().prettyURL(+1));

        KIO::Job* job = 0;
        switch (command.type()) {
            case DolphinCommand::Link:
            case DolphinCommand::Copy: {
                KUrl::List list;
                while (it != end) {
                    const KUrl deleteURL(destURL + (*it).filename());
                    list.append(deleteURL);
                    ++it;
                }
                job = KIO::del(list, false, false);
                break;
            }

            case DolphinCommand::Move: {
                KUrl::List list;
                const KUrl newDestURL((*it).directory());
                while (it != end) {
                    const KUrl newSourceURL(destURL + (*it).filename());
                    list.append(newSourceURL);
                    ++it;
                }
                job = KIO::move(list, newDestURL, false);
                break;
            }

            case DolphinCommand::Rename: {
                assert(sourceURLs.count() == 1);
                KIO::NetAccess::move(command.destination(), (*it));
                break;
            }

            case DolphinCommand::Trash: {
                while (it != end) {
                    // TODO: use KIO::special for accessing the trash protocol. See
                    // also Dolphin::slotJobResult() for further details.
                    const QString originalFileName((*it).filename().section('-', 1));
                    KUrl newDestURL(destURL + originalFileName);
                    KIO::NetAccess::move(*it, newDestURL);
                    ++it;

                    m_progressIndicator->execOperation();
                }
                break;
            }

            case DolphinCommand::CreateFolder:
            case DolphinCommand::CreateFile: {
                KIO::NetAccess::del(command.destination(), &Dolphin::mainWin());
                break;
            }
        }

        if (job != 0) {
            // Execute the jobs in a synchronous manner and forward the progress
            // information to the Dolphin statusbar.
            connect(job, SIGNAL(percent(KIO::Job*, unsigned long)),
                    this, SLOT(slotPercent(KIO::Job*, unsigned long)));
            KIO::NetAccess::synchronousRun(job, &Dolphin::mainWin());
        }

        m_progressIndicator->execOperation();
    }

    delete m_progressIndicator;
    m_progressIndicator = 0;
}

void UndoManager::redo()
{
    if (m_recordMacro) {
        endMacro();
    }

    const int maxHistoryIndex = m_history.count() - 1;
    if (m_historyIndex >= maxHistoryIndex) {
        return;
    }
    ++m_historyIndex;

    int progressCount = 0;
    int macroCount = 1;
    calcStepsCount(macroCount, progressCount);

    m_progressIndicator = new ProgressIndicator(i18n("Executing redo operation..."),
                                                i18n("Executed redo operation."),
                                                progressCount);

    for (int i = 0; i < macroCount; ++i) {
        const DolphinCommand command = m_history[m_historyIndex];
        if (m_historyIndex >= maxHistoryIndex) {
            emit redoAvailable(false);
            emit redoTextChanged(i18n("Redo"));
        }
        else {
            emit redoTextChanged(i18n("Redo: %1").arg(commandText(m_history[m_historyIndex + 1])));
        }

        emit undoAvailable(true);
        emit undoTextChanged(i18n("Undo: %1").arg(commandText(command)));

        Dolphin& dolphin = Dolphin::mainWin();

        KUrl::List sourceURLs = command.source();
        KUrl::List::Iterator it = sourceURLs.begin();
        const KUrl::List::Iterator end = sourceURLs.end();

        KIO::Job* job = 0;
        switch (command.type()) {
            case DolphinCommand::Link: {
                job = KIO::link(sourceURLs, command.destination(), false);
                break;
            }

            case DolphinCommand::Copy: {
                job = KIO::copy(sourceURLs, command.destination(), false);
                break;
            }

            case DolphinCommand::Rename:
            case DolphinCommand::Move: {
                job = KIO::move(sourceURLs, command.destination(), false);
                break;
            }

            case DolphinCommand::Trash: {
                const QString destURL(command.destination().prettyURL());
                while (it != end) {
                   // TODO: use KIO::special for accessing the trash protocol. See
                    // also Dolphin::slotJobResult() for further details.
                    const QString originalFileName((*it).filename().section('-', 1));
                    KUrl originalSourceURL(destURL + "/" + originalFileName);
                    KIO::Job* moveToTrashJob = KIO::trash(originalSourceURL);
                    KIO::NetAccess::synchronousRun(moveToTrashJob, &dolphin);
                    ++it;

                    m_progressIndicator->execOperation();
                 }
                break;
            }

            case DolphinCommand::CreateFolder: {
                KIO::NetAccess::mkdir(command.destination(), &dolphin);
                break;
            }

            case DolphinCommand::CreateFile: {
                m_progressIndicator->execOperation();
                KUrl::List::Iterator it = sourceURLs.begin();
                assert(sourceURLs.count() == 1);
                KIO::CopyJob* copyJob = KIO::copyAs(*it, command.destination(), false);
                copyJob->setDefaultPermissions(true);
                job = copyJob;
                break;
            }
        }

        if (job != 0) {
            // Execute the jobs in a synchronous manner and forward the progress
            // information to the Dolphin statusbar.
            connect(job, SIGNAL(percent(KIO::Job*, unsigned long)),
                    this, SLOT(slotPercent(KIO::Job*, unsigned long)));
            KIO::NetAccess::synchronousRun(job, &dolphin);
        }

        ++m_historyIndex;
        m_progressIndicator->execOperation();
    }

    --m_historyIndex;

    delete m_progressIndicator;
    m_progressIndicator = 0;
}

UndoManager::UndoManager() :
    m_recordMacro(false),
    m_historyIndex(-1),
    m_macroCounter(0),
    m_progressIndicator(0)
{
}

UndoManager::~UndoManager()
{
    delete m_progressIndicator;
    m_progressIndicator = 0;
}

QString UndoManager::commandText(const DolphinCommand& command) const
{
    QString text;
    switch (command.type()) {
        case DolphinCommand::Copy:         text = i18n("Copy"); break;
        case DolphinCommand::Move:         text = i18n("Move"); break;
        case DolphinCommand::Link:         text = i18n("Link"); break;
        case DolphinCommand::Rename:       text = i18n("Rename"); break;
        case DolphinCommand::Trash:        text = i18n("Move to Trash"); break;
        case DolphinCommand::CreateFolder: text = i18n("Create New Folder"); break;
        case DolphinCommand::CreateFile:   text = i18n("Create New File"); break;
        default: break;
    }
    return text;
}

void UndoManager::slotPercent(KIO::Job* /* job */, unsigned long /* percent */)
{
    // It is not allowed to update the progress indicator in the context
    // of this slot, hence do an asynchronous triggering.
    QTimer::singleShot(0, this, SLOT(updateProgress()));
}

void UndoManager::updateProgress()
{
    m_progressIndicator->execOperation();
}

void UndoManager::calcStepsCount(int& macroCount, int& progressCount)
{
    progressCount = 0;
    macroCount = 0;

    const int macroIndex = m_history[m_historyIndex].m_macroIndex;
    if (macroIndex < 0) {
        // default use case: no macro has been recorded
        macroCount = 1;
        progressCount = m_history[m_historyIndex].source().count();
        return;
    }

    // iterate backward for undo...
    int i = m_historyIndex;
    while ((i >= 0) && (m_history[i].m_macroIndex == macroIndex)) {
        ++macroCount;
        progressCount += m_history[i].source().count();
        --i;
    }

    // iterate forward for redo...
    const int max = m_history.count() - 1;
    i = m_historyIndex + 1;
    while ((i <= max) && (m_history[i].m_macroIndex == macroIndex)) {
        ++macroCount;
        progressCount += m_history[i].source().count();
        ++i;
    }
}

#include "undomanager.moc"


