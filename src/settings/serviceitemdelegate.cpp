/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "serviceitemdelegate.h"

#include <KDebug>
#include <KPushButton>

#include "servicemodel.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QModelIndex>
#include <QPainter>

ServiceItemDelegate::ServiceItemDelegate(QAbstractItemView* itemView, QObject* parent) :
    KWidgetItemDelegate(itemView, parent)
{
}

ServiceItemDelegate::~ServiceItemDelegate()
{
}

QSize ServiceItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    Q_UNUSED(index);

    const QStyle *style = itemView()->style();
    const int buttonHeight = style->pixelMetric(QStyle::PM_ButtonMargin) * 2 +
                             style->pixelMetric(QStyle::PM_ButtonIconSize);
    const int fontHeight = option.fontMetrics.height();
    return QSize(100, qMax(buttonHeight, fontHeight));
}

void ServiceItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    Q_UNUSED(index);
    painter->save();

    itemView()->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.highlightedText().color());
    }

    painter->restore();
}

QList<QWidget*> ServiceItemDelegate::createItemWidgets() const
{
    QCheckBox* checkBox = new QCheckBox();
    connect(checkBox, SIGNAL(clicked(bool)), this, SLOT(slotCheckBoxClicked(bool)));

    KPushButton* configureButton = new KPushButton();
    connect(configureButton, SIGNAL(clicked()), this, SLOT(slotConfigureButtonClicked()));

    return QList<QWidget*>() << checkBox << configureButton;
}

void ServiceItemDelegate::updateItemWidgets(const QList<QWidget*> widgets,
                                              const QStyleOptionViewItem& option,
                                              const QPersistentModelIndex& index) const
{
    QCheckBox* checkBox = static_cast<QCheckBox*>(widgets[0]);
    KPushButton *configureButton = static_cast<KPushButton*>(widgets[1]);

    const int itemHeight = sizeHint(option, index).height();

    // Update the checkbox showing the service name and icon
    const QAbstractItemModel* model = index.model();
    checkBox->setText(model->data(index).toString());
    const QString iconName = model->data(index, Qt::DecorationRole).toString();
    if (!iconName.isEmpty()) {
        checkBox->setIcon(KIcon(iconName));
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
        configureButton->setIcon(KIcon("configure"));
        configureButton->resize(configureButton->sizeHint());
        configureButton->move(option.rect.right() - configureButton->width(),
                              (itemHeight - configureButton->height()) / 2);
    }
    configureButton->setVisible(configurable);
}

void ServiceItemDelegate::slotCheckBoxClicked(bool checked)
{
    QAbstractItemModel* model = const_cast<QAbstractItemModel*>(focusedIndex().model());
    model->setData(focusedIndex(), checked, Qt::CheckStateRole);
}

void ServiceItemDelegate::slotConfigureButtonClicked()
{
    emit requestServiceConfiguration(focusedIndex());
}

#include "serviceitemdelegate.moc"
