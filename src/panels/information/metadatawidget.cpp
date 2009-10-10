/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "metadatawidget.h"

#include <kfileitem.h>
#include <klocale.h>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
#include "commentwidget_p.h"
#include "nepomukmassupdatejob_p.h"
#include "taggingwidget_p.h"
#endif

#include <nepomuk/kratingwidget.h>

#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QString>

class MetaDataWidget::Private
{
public:
    struct Row
    {
        QLabel* label;
        QWidget* infoWidget;
    };

    Private(MetaDataWidget* parent);
    ~Private();

    void addRow(QLabel* label, QWidget* infoWidget);
    void setRowVisible(QWidget* infoWidget, bool visible);

    QList<Row> m_rows;

    QGridLayout* m_gridLayout;

    QLabel* m_typeInfo;
    QLabel* m_sizeLabel;
    QLabel* m_sizeInfo;
    QLabel* m_modifiedInfo;
    QLabel* m_ownerInfo;
    QLabel* m_permissionsInfo;
#ifdef HAVE_NEPOMUK
    KRatingWidget* m_ratingWidget;
    TaggingWidget* m_taggingWidget;
    CommentWidget* m_commentWidget;
#endif

private:
    MetaDataWidget* const q;
};

MetaDataWidget::Private::Private(MetaDataWidget* parent) :
    m_rows(),
    m_gridLayout(0),
    m_typeInfo(0),
    m_sizeLabel(0),
    m_sizeInfo(0),
    m_modifiedInfo(0),
    m_ownerInfo(0),
    m_permissionsInfo(0),
#ifdef HAVE_NEPOMUK
    m_ratingWidget(0),
    m_taggingWidget(0),
    m_commentWidget(0),
#endif
    q(parent)
{
    m_gridLayout = new QGridLayout(parent);

    m_typeInfo = new QLabel(parent);
    m_sizeLabel = new QLabel(parent);
    m_sizeInfo = new QLabel(parent);
    m_modifiedInfo = new QLabel(parent);
    m_ownerInfo = new QLabel(parent);
    m_permissionsInfo = new QLabel(parent);
#ifdef HAVE_NEPOMUK
    m_ratingWidget = new KRatingWidget(parent);
    m_taggingWidget = new TaggingWidget(parent);
    m_commentWidget = new CommentWidget(parent);
#endif

    addRow(new QLabel(i18nc("@label", "Type:"), parent), m_typeInfo);
    addRow(m_sizeLabel, m_sizeInfo);
    addRow(new QLabel(i18nc("@label", "Modified:"), parent), m_modifiedInfo);
    addRow(new QLabel(i18nc("@label", "Owner:"), parent), m_ownerInfo);
    addRow(new QLabel(i18nc("@label", "Permissions:"), parent), m_permissionsInfo);
#ifdef HAVE_NEPOMUK
    addRow(new QLabel(i18nc("@label", "Rating:"), parent), m_ratingWidget);
    addRow(new QLabel(i18nc("@label", "Tags:"), parent), m_taggingWidget);
    addRow(new QLabel(i18nc("@label", "Comment:"), parent), m_commentWidget);
#endif
}

MetaDataWidget::Private::~Private()
{
}

void MetaDataWidget::Private::addRow(QLabel* label, QWidget* infoWidget)
{
    Row row;
    row.label = label;
    row.infoWidget = infoWidget;
    m_rows.append(row);

    QPalette palette = label->palette();
    QColor textColor = palette.color(QPalette::Text);
    textColor.setAlpha(128);
    palette.setColor(QPalette::WindowText, textColor);
    label->setPalette(palette);

    const int rowIndex = m_rows.count();
    m_gridLayout->addWidget(label, rowIndex, 0, Qt::AlignLeft);
    m_gridLayout->addWidget(infoWidget, rowIndex, 1, Qt::AlignRight);
}

void MetaDataWidget::Private::setRowVisible(QWidget* infoWidget, bool visible)
{
    foreach (const Row& row, m_rows) {
        if (row.infoWidget == infoWidget) {
            row.label->setVisible(visible);
            row.infoWidget->setVisible(visible);
            return;
        }
    }
}


MetaDataWidget::MetaDataWidget(QWidget* parent) :
    QWidget(parent),
    d(new Private(this))
{
}

MetaDataWidget::~MetaDataWidget()
{
    delete d;
}

void MetaDataWidget::setItem(const KFileItem& item)
{
    // update values for "type", "size", "modified",
    // "owner" and "permissions" synchronously
    d->m_sizeLabel->setText(i18nc("@label", "Size:"));
    if (item.isDir()) {
        d->m_typeInfo->setText(i18nc("@label", "Folder"));
        d->setRowVisible(d->m_sizeInfo, false);
    } else {
        d->m_typeInfo->setText(item.mimeComment());
        d->m_sizeInfo->setText(KIO::convertSize(item.size()));
        d->setRowVisible(d->m_sizeInfo, true);
    }
    d->m_modifiedInfo->setText(item.timeString());
    d->m_ownerInfo->setText(item.user());
    d->m_permissionsInfo->setText(item.permissionsString());

    setItems(KFileItemList() << item);
}

void MetaDataWidget::setItems(const KFileItemList& items)
{
    if (items.count() > 1) {
        // calculate the size of all items and show this
        // information to the user
        d->m_sizeLabel->setText(i18nc("@label", "Total Size:"));
        d->setRowVisible(d->m_sizeInfo, true);

        quint64 totalSize = 0;
        foreach (const KFileItem& item, items) {
            if (!item.isDir() && !item.isLink()) {
                totalSize += item.size();
            }
        }
        d->m_sizeInfo->setText(KIO::convertSize(totalSize));
    }
}

#include "metadatawidget.moc"
