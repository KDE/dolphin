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
    void growsWiderWhileANameThatDoesNotWrapIsTyped();
    void growsNoWiderThanTheView();
    void keepsItsWidthWhileANameThatWrapsIsTyped();

private:
    void sendFocusOut(Qt::FocusReason reason);
    // Places the editor over a short name, as the view does, with the wrap mode of the name.
    void startEditing(QTextOption::WrapMode wrapMode);

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

void KItemListRoleEditorTest::startEditing(QTextOption::WrapMode wrapMode)
{
    QTextOption textOption = m_editor->document()->defaultTextOption();
    textOption.setWrapMode(wrapMode);
    m_editor->document()->setDefaultTextOption(textOption);
    m_editor->setPlainText(QStringLiteral("a"));
    m_editor->setGeometry(10, 10, 30, 30);
}

// The compact view shows a name on one line. See bug 524922.
void KItemListRoleEditorTest::growsWiderWhileANameThatDoesNotWrapIsTyped()
{
    startEditing(QTextOption::NoWrap);

    m_editor->insertPlainText(QStringLiteral("bcdef"));

    QVERIFY2(m_editor->width() > 30, qPrintable(QString::number(m_editor->width())));
    QVERIFY(m_editor->width() < m_parent->width() - m_editor->x());
}

void KItemListRoleEditorTest::growsNoWiderThanTheView()
{
    startEditing(QTextOption::NoWrap);

    m_editor->insertPlainText(QStringLiteral("a name much longer than the view it is typed in"));

    QCOMPARE(m_editor->geometry().right() + 1, m_parent->width());
}

// The icons view wraps a name, and the editor gets taller instead.
void KItemListRoleEditorTest::keepsItsWidthWhileANameThatWrapsIsTyped()
{
    startEditing(QTextOption::WrapAtWordBoundaryOrAnywhere);

    m_editor->insertPlainText(QStringLiteral(" a longer name that wraps"));

    QCOMPARE(m_editor->width(), 30);
    QVERIFY(m_editor->height() > 30);
}

QTEST_MAIN(KItemListRoleEditorTest)

#include "kitemlistroleeditortest.moc"
