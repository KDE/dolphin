/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistroleeditor.h"

#include <KIO/Global>

KItemListRoleEditor::KItemListRoleEditor(QWidget *parent) :
    KTextEdit(parent),
    m_role(),
    m_blockFinishedSignal(false)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAcceptRichText(false);
    enableFindReplace(false);
    document()->setDocumentMargin(0);

    if (parent) {
        parent->installEventFilter(this);
    }

    connect(this, &KItemListRoleEditor::textChanged, this, &KItemListRoleEditor::autoAdjustSize);
}

KItemListRoleEditor::~KItemListRoleEditor()
{
}

void KItemListRoleEditor::setRole(const QByteArray& role)
{
    m_role = role;
}

QByteArray KItemListRoleEditor::role() const
{
    return m_role;
}

void KItemListRoleEditor::setAllowUpDownKeyChainEdit(bool allowChainEdit)
{
    m_allowUpDownKeyChainEdit = allowChainEdit;
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
    if (event->type() == QEvent::FocusOut) {
        QFocusEvent* focusEvent = static_cast<QFocusEvent*>(event);
        if (focusEvent->reason() != Qt::PopupFocusReason) {
            emitRoleEditingFinished();
        }
    }
    return KTextEdit::event(event);
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
        Q_EMIT roleEditingCanceled(m_role, KIO::encodeFileName(toPlainText()));
        m_blockFinishedSignal = false;
        event->accept();
        return;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        emitRoleEditingFinished();
        event->accept();
        return;
    case Qt::Key_Tab:
    case Qt::Key_Down:
        if (m_allowUpDownKeyChainEdit || event->key() == Qt::Key_Tab) {
            emitRoleEditingFinished(EditNext);
            event->accept();
            return;
        }
        break;
    case Qt::Key_Backtab:
    case Qt::Key_Up:
        if (m_allowUpDownKeyChainEdit || event->key() == Qt::Key_Backtab) {
            emitRoleEditingFinished(EditPrevious);
            event->accept();
            return;
        }
        break;
    case Qt::Key_Left:
    case Qt::Key_Right: {
        QTextCursor cursor = textCursor();
        if (event->modifiers() == Qt::NoModifier && cursor.hasSelection()) {
            if (event->key() == Qt::Key_Left) {
                cursor.setPosition(cursor.selectionStart());
            } else {
                cursor.setPosition(cursor.selectionEnd());
            }
            cursor.clearSelection();
            setTextCursor(cursor);
            event->accept();
            return;
        }
        break;
    }
    case Qt::Key_Home:
    case Qt::Key_End: {
        if (event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::ShiftModifier) {
            const QTextCursor::MoveOperation op = event->key() == Qt::Key_Home
                                                ? QTextCursor::Start
                                                : QTextCursor::End;
            const QTextCursor::MoveMode mode = event->modifiers() == Qt::NoModifier
                                             ? QTextCursor::MoveAnchor
                                             : QTextCursor::KeepAnchor;
            QTextCursor cursor = textCursor();
            cursor.movePosition(op, mode);
            setTextCursor(cursor);
            event->accept();
            return;
        }
        break;
    }
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

void KItemListRoleEditor::emitRoleEditingFinished(EditResultDirection direction)
{
    QVariant ret;
    ret.setValue(EditResult {KIO::encodeFileName(toPlainText()), direction});

    if (!m_blockFinishedSignal) {
        Q_EMIT roleEditingFinished(m_role, ret);
    }
}

