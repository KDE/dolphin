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

#ifndef KITEMLISTROLEEDITOR_H
#define KITEMLISTROLEEDITOR_H

#include "dolphin_export.h"

#include <KTextEdit>

/**
 * @brief Editor for renaming roles of a KItemListWidget.
 *
 * Provides signals when the editing got cancelled (e.g. by
 * pressing Escape or when losing the focus) or when the editing
 * got finished (e.g. by pressing Enter or Return).
 *
 * The size automatically gets increased if the text does not fit.
 */
class DOLPHIN_EXPORT KItemListRoleEditor : public KTextEdit
{
    Q_OBJECT

public:
    explicit KItemListRoleEditor(QWidget* parent);
    virtual ~KItemListRoleEditor();

    void setRole(const QByteArray& role);
    QByteArray role() const;

    bool eventFilter(QObject* watched, QEvent* event) Q_DECL_OVERRIDE;

signals:
    void roleEditingFinished(const QByteArray& role, const QVariant& value);
    void roleEditingCanceled(const QByteArray& role, const QVariant& value);

protected:
    bool event(QEvent* event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent* event) Q_DECL_OVERRIDE;

private slots:
    /**
     * Increases the size of the editor in case if there is not
     * enough room for the text.
     */
    void autoAdjustSize();

private:
    /**
     * Emits the signal roleEditingFinished if m_blockFinishedSignal
     * is false.
     */
    void emitRoleEditingFinished();

private:
    QByteArray m_role;
    bool m_blockFinishedSignal;
};

#endif
