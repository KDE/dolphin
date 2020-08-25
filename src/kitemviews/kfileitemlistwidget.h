/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KFILEITEMLISTWIDGET_H
#define KFILEITEMLISTWIDGET_H

#include "dolphin_export.h"
#include "kitemviews/kstandarditemlistwidget.h"

class DOLPHIN_EXPORT KFileItemListWidgetInformant : public KStandardItemListWidgetInformant
{
public:
    KFileItemListWidgetInformant();
    ~KFileItemListWidgetInformant() override;

protected:
    QString itemText(int index, const KItemListView* view) const override;
    bool itemIsLink(int index, const KItemListView* view) const override;
    QString roleText(const QByteArray& role, const QHash<QByteArray, QVariant>& values) const override;
    QFont customizedFontForLinks(const QFont& baseFont) const override;
};

class DOLPHIN_EXPORT KFileItemListWidget : public KStandardItemListWidget
{
    Q_OBJECT

public:
    KFileItemListWidget(KItemListWidgetInformant* informant, QGraphicsItem* parent);
    ~KFileItemListWidget() override;

    static KItemListWidgetInformant* createInformant();

protected:
    bool isRoleRightAligned(const QByteArray& role) const override;
    bool isHidden() const override;
    QFont customizedFont(const QFont& baseFont) const override;

    /**
     * @return Selection length without MIME-type extension
     */
    int selectionLength(const QString& text) const override;
};

#endif


