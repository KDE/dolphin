/*
 * SPDX-FileCopyrightText: 2026 Meven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/private/kitemlistroleeditor.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

class KItemListRoleEditorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void keepsEditingWhileAnotherWindowHasTheFocus();
    void keepsEditingWhileAMenuHasTheFocus();
    void finishesEditingWhenTheFocusMovesOnInTheSameWindow();

private:
    void sendFocusOut(Qt::FocusReason reason);

    QWidget *m_parent = nullptr;
    KItemListRoleEditor *m_editor = nullptr;
};

void KItemListRoleEditorTest::init()
{
    m_parent = new QWidget();
    m_parent->resize(200, 100);
    m_editor = new KItemListRoleEditor(m_parent);
    m_editor->setRole(QByteArrayLiteral("text"));
    m_editor->setPlainText(QStringLiteral("a name being typed"));
    m_parent->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_parent));
    m_editor->setFocus();
}

void KItemListRoleEditorTest::cleanup()
{
    delete m_parent;
    m_parent = nullptr;
    m_editor = nullptr;
}

// The on-screen keyboard of a touchscreen is a window of its own, and it takes the keyboard focus
// as soon as it appears over the name being typed. The name stays under edit. See bug 470238.
void KItemListRoleEditorTest::keepsEditingWhileAnotherWindowHasTheFocus()
{
    QSignalSpy editingFinished(m_editor, &KItemListRoleEditor::roleEditingFinished);
    QSignalSpy editingCanceled(m_editor, &KItemListRoleEditor::roleEditingCanceled);

    sendFocusOut(Qt::ActiveWindowFocusReason);

    QCOMPARE(editingFinished.count(), 0);
    QCOMPARE(editingCanceled.count(), 0);
}

void KItemListRoleEditorTest::keepsEditingWhileAMenuHasTheFocus()
{
    QSignalSpy editingFinished(m_editor, &KItemListRoleEditor::roleEditingFinished);

    sendFocusOut(Qt::PopupFocusReason);

    QCOMPARE(editingFinished.count(), 0);
}

// Turning to anything else in the same window is the user leaving the name alone, and what was
// typed is applied.
void KItemListRoleEditorTest::finishesEditingWhenTheFocusMovesOnInTheSameWindow()
{
    QSignalSpy editingFinished(m_editor, &KItemListRoleEditor::roleEditingFinished);

    sendFocusOut(Qt::MouseFocusReason);

    QCOMPARE(editingFinished.count(), 1);
    QCOMPARE(editingFinished.first().at(0).toByteArray(), QByteArrayLiteral("text"));
    QCOMPARE(editingFinished.first().at(1).value<EditResult>().newName, QStringLiteral("a name being typed"));
}

void KItemListRoleEditorTest::sendFocusOut(Qt::FocusReason reason)
{
    QFocusEvent focusOut(QEvent::FocusOut, reason);
    QCoreApplication::sendEvent(m_editor, &focusOut);
}

QTEST_MAIN(KItemListRoleEditorTest)

#include "kitemlistroleeditortest.moc"
