/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kitemlistroleeditor.h"

#include <KDebug>
#include <QKeyEvent>

KItemListRoleEditor::KItemListRoleEditor(QWidget *parent) :
    KTextEdit(parent),
    m_index(0),
    m_role(),
    m_blockFinishedSignal(false),
    m_eventHandlingLevel(0),
    m_deleteAfterEventHandling(false)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAcceptRichText(false);
    document()->setDocumentMargin(0);

    if (parent) {
        parent->installEventFilter(this);
    }

    connect(this, SIGNAL(textChanged()), this, SLOT(autoAdjustSize()));
}

KItemListRoleEditor::~KItemListRoleEditor()
{
}

void KItemListRoleEditor::setIndex(int index)
{
    m_index = index;
}

int KItemListRoleEditor::index() const
{
    return m_index;
}

void KItemListRoleEditor::setRole(const QByteArray& role)
{
    m_role = role;
}

QByteArray KItemListRoleEditor::role() const
{
    return m_role;
}

void KItemListRoleEditor::deleteWhenIdle()
{
    if (m_eventHandlingLevel > 0) {
        // We are handling an event at the moment. It could be that we
        // are in a nested event loop run by contextMenuEvent() or a
        // call of mousePressEvent() which results in drag&drop.
        // -> do not call deleteLater() to prevent a crash when we
        //    return from the nested event loop.
        m_deleteAfterEventHandling = true;
    } else {
        deleteLater();
    }
}

bool KItemListRoleEditor::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        emitRoleEditingFinished();
    }

    return KTextEdit::eventFilter(watched, event);
}

bool KItemListRoleEditor::event(QEvent* event)
{
    ++m_eventHandlingLevel;

    if (event->type() == QEvent::FocusOut) {
        QFocusEvent* focusEvent = static_cast<QFocusEvent*>(event);
        if (focusEvent->reason() != Qt::PopupFocusReason) {
            emitRoleEditingFinished();
        }
    }

    const int result = KTextEdit::event(event);
    --m_eventHandlingLevel;

    if (m_deleteAfterEventHandling && m_eventHandlingLevel == 0) {
        // Schedule this object for deletion and make sure that we do not try
        // to deleteLater() again when the DeferredDelete event is received.
        deleteLater();
        m_deleteAfterEventHandling = false;
    }

    return result;
}

bool KItemListRoleEditor::viewportEvent(QEvent* event)
{
    ++m_eventHandlingLevel;
    const bool result = KTextEdit::viewportEvent(event);
    --m_eventHandlingLevel;

    if (m_deleteAfterEventHandling && m_eventHandlingLevel == 0) {
        // Schedule this object for deletion and make sure that we do not try
        // to deleteLater() again when the DeferredDelete event is received.
        deleteLater();
        m_deleteAfterEventHandling = false;
    }

    return result;
}

void KItemListRoleEditor::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        // Emitting the signal roleEditingCanceled might result
        // in losing the focus. Per default losing the focus emits
        // a roleEditingFinished signal (see KItemListRoleEditor::event),
        // which is not wanted in this case.
        m_blockFinishedSignal = true;
        emit roleEditingCanceled(m_index, m_role, toPlainText());
        m_blockFinishedSignal = false;
        event->accept();
        return;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        emitRoleEditingFinished();
        event->accept();
        return;
    default:
        break;
    }

    KTextEdit::keyPressEvent(event);
}

void KItemListRoleEditor::autoAdjustSize()
{
    const qreal frameBorder = 2 * frameWidth();

    const qreal requiredWidth = document()->size().width();
    const qreal availableWidth = size().width() - frameBorder;
    if (requiredWidth > availableWidth) {
        qreal newWidth = requiredWidth + frameBorder;
        if (parentWidget() && pos().x() + newWidth > parentWidget()->width()) {
            newWidth = parentWidget()->width() - pos().x();
        }
        resize(newWidth, size().height());
    }

    const qreal requiredHeight = document()->size().height();
    const qreal availableHeight = size().height() - frameBorder;
    if (requiredHeight > availableHeight) {
        qreal newHeight = requiredHeight + frameBorder;
        if (parentWidget() && pos().y() + newHeight > parentWidget()->height()) {
            newHeight = parentWidget()->height() - pos().y();
        }
        resize(size().width(), newHeight);
    }
}

void KItemListRoleEditor::emitRoleEditingFinished()
{
    if (!m_blockFinishedSignal) {
        emit roleEditingFinished(m_index, m_role, toPlainText());
    }
}

#include "kitemlistroleeditor.moc"
