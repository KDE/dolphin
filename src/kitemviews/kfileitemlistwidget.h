/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KFILEITEMLISTWIDGET_H
#define KFILEITEMLISTWIDGET_H

#include "dolphin_export.h"
#include "kitemviews/kstandarditemlistwidget.h"

class KFileItemListView;

/**
 * @brief ItemList widget informant implementation for use with KFileItems.
 *
 * Code that does not expect KFileItems specifically should go to KStandardItemListWidgetInformant.
 *
 * @see KItemListWidgetInformant
 */
class DOLPHIN_EXPORT KFileItemListWidgetInformant : public KStandardItemListWidgetInformant
{
public:
    KFileItemListWidgetInformant();
    ~KFileItemListWidgetInformant() override;

protected:
    QString itemText(int index, const KItemListView *view) const override;
    bool itemIsLink(int index, const KItemListView *view) const override;
    /** @see KStandardItemListWidget::roleText(). */
    QString roleText(const QByteArray &role, const QHash<QByteArray, QVariant> &values, ForUsageAs forUsageAs = ForUsageAs::DisplayedText) const override;
    QFont customizedFontForLinks(const QFont &baseFont) const override;

    friend class KItemListDelegateAccessible;
};

/**
 * @brief ItemList widget implementation for use with KFileItems.
 *
 * Code that does not expect KFileItems specifically should go to KStandardItemListWidget.
 *
 * @see KItemListWidget
 */
class DOLPHIN_EXPORT KFileItemListWidget : public KStandardItemListWidget
{
    Q_OBJECT

public:
    KFileItemListWidget(KItemListWidgetInformant *informant, QGraphicsItem *parent);
    ~KFileItemListWidget() override;

    static KItemListWidgetInformant *createInformant();

    /// Force-update the displayed icon
    void forceUpdate();

protected:
    virtual void hoverSequenceStarted() override;
    virtual void hoverSequenceIndexChanged(int sequenceIndex) override;
    virtual void hoverSequenceEnded() override;

    bool isRoleRightAligned(const QByteArray &role) const override;
    bool isHidden() const override;
    QFont customizedFont(const QFont &baseFont) const override;

    /**
     * @return Selection length without MIME-type extension in number of unicode characters, which might be different from number of QChars.
     */
    int selectionLength(const QString &text) const override;

private:
    KFileItemListView *listView();
};

#endif
