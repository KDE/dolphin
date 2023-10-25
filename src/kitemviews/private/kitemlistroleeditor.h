/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTROLEEDITOR_H
#define KITEMLISTROLEEDITOR_H

#include "dolphin_export.h"

#include <KTextEdit>

enum EditResultDirection {
    EditDone,
    EditNext,
    EditPrevious,
};
Q_DECLARE_METATYPE(EditResultDirection)

struct EditResult {
    QString newName;
    EditResultDirection direction;
};
Q_DECLARE_METATYPE(EditResult)

/**
 * @brief Editor for renaming roles of a KItemListWidget.
 *
 * Provides signals when the editing got cancelled (e.g. by
 * pressing Escape or when losing the focus) or when the editing
 * got finished (e.g. by pressing Enter, Tab or Return).
 *
 * The size automatically gets increased if the text does not fit.
 */
class DOLPHIN_EXPORT KItemListRoleEditor : public KTextEdit
{
    Q_OBJECT

public:
    explicit KItemListRoleEditor(QWidget *parent);
    ~KItemListRoleEditor() override;

    void setRole(const QByteArray &role);
    QByteArray role() const;

    void setAllowUpDownKeyChainEdit(bool allowChainEdit);
    bool eventFilter(QObject *watched, QEvent *event) override;

Q_SIGNALS:
    void roleEditingFinished(const QByteArray &role, const QVariant &value);
    void roleEditingCanceled(const QByteArray &role, const QVariant &value);

public Q_SLOTS:
    /**
     * Increases the size of the editor in case if there is not
     * enough room for the text.
     */
    void autoAdjustSize();

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    /**
     * Emits the signal roleEditingFinished if m_blockFinishedSignal
     * is false.
     */
    void emitRoleEditingFinished(EditResultDirection direction = EditDone);

private:
    QByteArray m_role;
    bool m_blockFinishedSignal;
    bool m_allowUpDownKeyChainEdit;
};

#endif
