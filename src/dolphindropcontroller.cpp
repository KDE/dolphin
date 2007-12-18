/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2007 by David Faure <faure@kde.org>                     *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "dolphindropcontroller.h"
#include <klocale.h>
#include <kicon.h>
#include <QApplication>
#include <kdebug.h>
#include <kmenu.h>
#include <konq_operations.h>

// TODO replace with KonqOperations::doDrop [or move doDrop code into this class]
// Note that this means changing the DolphinDropController controller usage
// to "create with new and let it autodelete" instead of on stack, since doDrop is async.

DolphinDropController::DolphinDropController(QWidget* parentWidget)
    : QObject(parentWidget), m_parentWidget(parentWidget)
{
}

DolphinDropController::~DolphinDropController()
{
}

void DolphinDropController::dropUrls(const KUrl::List& urls,
                                     const KUrl& destination)
{
    kDebug() << "Source" << urls;
    kDebug() << "Destination:" << destination;

    Qt::DropAction action = Qt::CopyAction;

    Qt::KeyboardModifiers modifier = QApplication::keyboardModifiers();
    const bool shiftPressed   = modifier & Qt::ShiftModifier;
    const bool controlPressed = modifier & Qt::ControlModifier;
    const bool altPressed = modifier & Qt::AltModifier;
    if (shiftPressed && controlPressed) {
        // shortcut for 'Link Here' is used
        action = Qt::LinkAction;
    } else if (shiftPressed) {
        // shortcut for 'Move Here' is used
        action = Qt::MoveAction;
    } else if (controlPressed) {
        // shortcut for 'Copy Here' is used
        action = Qt::CopyAction;
    } else if (altPressed) {
        // shortcut for 'Link Here' is used
        action = Qt::LinkAction;
    } else {
        // open a context menu which offers the following actions:
        // - Move Here
        // - Copy Here
        // - Link Here
        // - Cancel

        KMenu popup(m_parentWidget);

        QString seq = QKeySequence(Qt::ShiftModifier).toString();
        seq.chop(1); // chop superfluous '+'
        QAction* moveAction = popup.addAction(KIcon("go-jump"),
                                              i18nc("@action:inmenu",
                                                    "&Move Here\t<shortcut>%1</shortcut>", seq));

        seq = QKeySequence(Qt::ControlModifier).toString();
        seq.chop(1);
        QAction* copyAction = popup.addAction(KIcon("edit-copy"),
                                              i18nc("@action:inmenu",
                                                    "&Copy Here\t<shortcut>%1</shortcut>", seq));

        seq = QKeySequence(Qt::ControlModifier + Qt::ShiftModifier).toString();
        seq.chop(1);
        QAction* linkAction = popup.addAction(KIcon("insert-link"),
                                              i18nc("@action:inmenu",
                                                    "&Link Here\t<shortcut>%1</shortcut>", seq));

        popup.addSeparator();
        popup.addAction(KIcon("process-stop"), i18nc("@action:inmenu", "Cancel"));

        QAction* activatedAction = popup.exec(QCursor::pos());
        if (activatedAction == moveAction) {
            action = Qt::MoveAction;
        } else if (activatedAction == copyAction) {
            action = Qt::CopyAction;
        } else if (activatedAction == linkAction) {
            action = Qt::LinkAction;
        } else {
            return;
        }
    }

    switch (action) {
    case Qt::MoveAction:
        KonqOperations::copy(m_parentWidget, KonqOperations::MOVE, urls, destination);
        emit doingOperation(KonqFileUndoManager::MOVE);
        break;

    case Qt::CopyAction:
        KonqOperations::copy(m_parentWidget, KonqOperations::COPY, urls, destination);
        emit doingOperation(KonqFileUndoManager::COPY);
        break;

    case Qt::LinkAction:
        KonqOperations::copy(m_parentWidget, KonqOperations::LINK, urls, destination);
        emit doingOperation(KonqFileUndoManager::LINK);
        break;

    default:
        break;
    }
}

#include "dolphindropcontroller.moc"
