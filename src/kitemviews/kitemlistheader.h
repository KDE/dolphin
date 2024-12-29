/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTHEADER_H
#define KITEMLISTHEADER_H

#include "dolphin_export.h"

#include <QHash>
#include <QObject>

class KItemListHeaderWidget;
class KItemListView;

/**
 * @brief Provides access to the header of a KItemListView.
 *
 * Each column of the header represents a visible role
 * accessible by KItemListView::visibleRoles().
 */
class DOLPHIN_EXPORT KItemListHeader : public QObject
{
    Q_OBJECT

public:
    ~KItemListHeader() override;

    /**
     * If set to true, KItemListView will automatically adjust the
     * widths of the columns. If set to false, the size can be
     * manually adjusted by KItemListHeader::setColumnWidth().
     */
    void setAutomaticColumnResizing(bool automatic);
    bool automaticColumnResizing() const;

    /**
     * Sets the width of the column for the given role. Note that
     * the width only gets applied if KItemListHeader::automaticColumnResizing()
     * has been turned off.
     */
    void setColumnWidth(const QByteArray &role, qreal width);
    qreal columnWidth(const QByteArray &role) const;

    /**
     * Sets the widths of the columns for all roles. From a performance point of
     * view calling this method should be preferred over several setColumnWidth()
     * calls in case if the width of more than one column should be changed.
     * Note that the widths only get applied if KItemListHeader::automaticColumnResizing()
     * has been turned off.
     */
    void setColumnWidths(const QHash<QByteArray, qreal> &columnWidths);

    /**
     * @return The column width that is required to show the role unclipped.
     */
    qreal preferredColumnWidth(const QByteArray &role) const;

    /**
     * Sets the widths of the columns *before* the first column and *after* the last column.
     * This is intended to facilitate an empty region for deselection in the main viewport.
     */
    void setSidePadding(qreal leftPaddingWidth, qreal rightPaddingWidth);
    qreal leftPadding() const;
    qreal rightPadding() const;

Q_SIGNALS:
    void sidePaddingChanged(qreal leftPaddingWidth, qreal rightPaddingWidth);

    /**
     * Is emitted if the width of a column has been adjusted by the user with the mouse
     * (no signal is emitted if KItemListHeader::setColumnWidth() is invoked).
     */
    void columnWidthChanged(const QByteArray &role, qreal currentWidth, qreal previousWidth);

    /**
     * Is emitted if the user has released the mouse button after adjusting the
     * width of a visible role.
     */
    void columnWidthChangeFinished(const QByteArray &role, qreal currentWidth);

private:
    explicit KItemListHeader(KItemListView *listView);

private:
    KItemListView *m_view;
    KItemListHeaderWidget *m_headerWidget;

    friend class KItemListView; // Constructs the KItemListHeader instance
};

#endif
