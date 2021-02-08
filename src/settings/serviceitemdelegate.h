/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SERVICEITEMDELEGATE_H
#define SERVICEITEMDELEGATE_H

#include <KWidgetItemDelegate>

/**
 * @brief Widget item delegate for a service that can be enabled or disabled.
 *
 * Additionally it is possible to configure a service.
 * @see ServiceModel
 */
class ServiceItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

public:
    explicit ServiceItemDelegate(QAbstractItemView* itemView, QObject* parent = nullptr);
    ~ServiceItemDelegate() override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const override;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
                       const QModelIndex& index) const override;

    QList<QWidget*> createItemWidgets(const QModelIndex&) const override;

    void updateItemWidgets(const QList<QWidget*> widgets,
                                   const QStyleOptionViewItem& option,
                                   const QPersistentModelIndex& index) const override;

Q_SIGNALS:
    void requestServiceConfiguration(const QModelIndex& index);

private Q_SLOTS:
    void slotCheckBoxClicked(bool checked);
    void slotConfigureButtonClicked();
};

#endif
