/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "serviceitemdelegate.h"

#include "servicemodel.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QPainter>
#include <QPushButton>

ServiceItemDelegate::ServiceItemDelegate(QAbstractItemView *itemView, QObject *parent)
    : KWidgetItemDelegate(itemView, parent)
{
}

ServiceItemDelegate::~ServiceItemDelegate()
{
}

QSize ServiceItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    const QStyle *style = itemView()->style();
    const int buttonHeight = style->pixelMetric(QStyle::PM_ButtonMargin) * 2 + style->pixelMetric(QStyle::PM_ButtonIconSize);
    const int fontHeight = option.fontMetrics.height();
    return QSize(100, qMax(buttonHeight, fontHeight));
}

void ServiceItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    painter->save();

    itemView()->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.highlightedText().color());
    }

    painter->restore();
}

QList<QWidget *> ServiceItemDelegate::createItemWidgets(const QModelIndex &) const
{
    QCheckBox *checkBox = new QCheckBox();
    QPalette palette = checkBox->palette();
    palette.setColor(QPalette::WindowText, palette.color(QPalette::Text));
    checkBox->setPalette(palette);
    connect(checkBox, &QCheckBox::clicked, this, &ServiceItemDelegate::slotCheckBoxClicked);

    QPushButton *configureButton = new QPushButton();
    connect(configureButton, &QPushButton::clicked, this, &ServiceItemDelegate::slotConfigureButtonClicked);

    return {checkBox, configureButton};
}

void ServiceItemDelegate::updateItemWidgets(const QList<QWidget *> &widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const
{
    QCheckBox *checkBox = static_cast<QCheckBox *>(widgets[0]);
    QPushButton *configureButton = static_cast<QPushButton *>(widgets[1]);

    const int itemHeight = sizeHint(option, index).height();

    // Update the checkbox showing the service name and icon
    const QAbstractItemModel *model = index.model();
    checkBox->setText(model->data(index).toString());
    const QString iconName = model->data(index, Qt::DecorationRole).toString();
    if (!iconName.isEmpty()) {
        checkBox->setIcon(QIcon::fromTheme(iconName));
    }
    checkBox->setChecked(model->data(index, Qt::CheckStateRole).toBool());

    const bool configurable = model->data(index, ServiceModel::ConfigurableRole).toBool();

    int checkBoxWidth = option.rect.width();
    if (configurable) {
        checkBoxWidth -= configureButton->sizeHint().width();
    }
    checkBox->resize(checkBoxWidth, checkBox->sizeHint().height());
    checkBox->move(0, (itemHeight - checkBox->height()) / 2);

    // Update the configuration button
    if (configurable) {
        configureButton->setEnabled(checkBox->isChecked());
        configureButton->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
        configureButton->resize(configureButton->sizeHint());
        configureButton->move(option.rect.right() - configureButton->width(), (itemHeight - configureButton->height()) / 2);
    }
    configureButton->setVisible(configurable);
}

void ServiceItemDelegate::slotCheckBoxClicked(bool checked)
{
    QAbstractItemModel *model = const_cast<QAbstractItemModel *>(focusedIndex().model());
    model->setData(focusedIndex(), checked, Qt::CheckStateRole);
}

void ServiceItemDelegate::slotConfigureButtonClicked()
{
    Q_EMIT requestServiceConfiguration(focusedIndex());
}
