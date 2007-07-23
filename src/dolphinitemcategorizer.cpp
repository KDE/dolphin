/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@gmail.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "dolphinitemcategorizer.h"

#include "dolphinview.h"
#include "dolphinsortfilterproxymodel.h"

#ifdef HAVE_NEPOMUK
#include <config-nepomuk.h>
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#endif

#include <kdatetime.h>
#include <kdirmodel.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kuser.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kpixmapeffect.h>

#include <QList>
#include <QSortFilterProxyModel>
#include <QPainter>
#include <QDir>

DolphinItemCategorizer::DolphinItemCategorizer() :
    KItemCategorizer()
{
}

DolphinItemCategorizer::~DolphinItemCategorizer()
{
}

QString DolphinItemCategorizer::categoryForItem(const QModelIndex& index,
                                                int sortRole) const
{
    QString retString;

    if (!index.isValid())
    {
        return retString;
    }

    const KDirModel *dirModel = qobject_cast<const KDirModel*>(index.model());
    KFileItem *item = dirModel->itemForIndex(index);

    switch (sortRole)
    {
        case DolphinView::SortByName:
        {
            // KDirModel checks columns to know to which role are
            // we talking about
            QModelIndex theIndex = index.model()->index(index.row(),
                                                        KDirModel::Name,
                                                        index.parent());

            if (!theIndex.isValid()) {
                return retString;
            }

            QVariant data = theIndex.model()->data(theIndex, Qt::DisplayRole);
            if (data.toString().size())
            {
                if (!item->isHidden() && data.toString().at(0).isLetter())
                    retString = data.toString().toUpper().at(0);
                else if (item->isHidden() && data.toString().at(0) == '.' &&
                         data.toString().at(1).isLetter())
                    retString = data.toString().toUpper().at(1);
                else if (item->isHidden() && data.toString().at(0) == '.' &&
                         !data.toString().at(1).isLetter())
                    retString = i18nc("@title:group Name", "Others");
                else if (item->isHidden() && data.toString().at(0) != '.')
                    retString = data.toString().toUpper().at(0);
                else if (item->isHidden())
                    retString = data.toString().toUpper().at(0);
                else
                {
                    bool validCategory = false;

                    const QString str(data.toString().toUpper());
                    const QChar* currA = str.unicode();
                    while (!currA->isNull() && !validCategory) {
                        if (currA->isLetter())
                            validCategory = true;
                        else if (currA->isDigit())
                            return i18nc("@title:group", "Others");
                        else
                            ++currA;
                    }

                    if (!validCategory)
                        retString = i18nc("@title:group Name", "Others");
                    else
                        retString = *currA;
                }
            }
            break;
        }

        case DolphinView::SortByDate:
        {
            KDateTime modifiedTime;
            modifiedTime.setTime_t(item->time(KIO::UDS_MODIFICATION_TIME));
            modifiedTime = modifiedTime.toLocalZone();

            if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) == 0)
                retString = i18nc("@title:group Date", "Today");
            else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) == 1)
                retString = i18nc("@title:group Date", "Yesterday");
            else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 7)
                retString = i18nc("@title:group Date", "Less than a week");
            else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 31)
                retString = i18nc("@title:group Date", "Less than a month");
            else if (modifiedTime.daysTo(KDateTime::currentLocalDateTime()) < 365)
                retString = i18nc("@title:group Date", "Less than a year");
            else
                retString = i18nc("@title:group Date", "More than a year");
            break;
        }

        case DolphinView::SortByPermissions:
            retString = item->permissionsString();
            break;

        case DolphinView::SortByOwner:
            retString = item->user();
            break;

        case DolphinView::SortByGroup:
            retString = item->group();
            break;

        case DolphinView::SortBySize: {
            const int fileSize = item ? item->size() : -1;
            if (item && item->isDir()) {
                retString = i18nc("@title:group Size", "Folders");
            } else if (fileSize < 5242880) {
                retString = i18nc("@title:group Size", "Small");
            } else if (fileSize < 10485760) {
                retString = i18nc("@title:group Size", "Medium");
            } else {
                retString = i18nc("@title:group Size", "Big");
            }
            break;
        }

        case DolphinView::SortByType:
            retString = item->mimeComment();
            break;

#ifdef HAVE_NEPOMUK
        case DolphinView::SortByRating: {
            const quint32 rating = DolphinSortFilterProxyModel::ratingForIndex(index);

            retString = QString::number(rating);
            break;
        }

        case DolphinView::SortByTags: {
            retString = DolphinSortFilterProxyModel::tagsForIndex(index);

            if (retString.isEmpty())
                retString = i18nc("@title:group Tags", "Not yet tagged");

            break;
        }
#endif
    }

    return retString;
}

void DolphinItemCategorizer::drawCategory(const QModelIndex &index,
                                          int sortRole,
                                          const QStyleOption &option,
                                          QPainter *painter) const
{
    QRect starRect = option.rect;

    int iconSize =  KIconLoader::global()->currentSize(K3Icon::Small);                       
    const QString category = categoryForItem(index, sortRole);

    QColor color = option.palette.color(QPalette::Text);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QStyleOptionButton opt;

    opt.rect = option.rect;
    opt.palette = option.palette;
    opt.direction = option.direction;
    opt.text = category;

    if (option.state & QStyle::State_MouseOver)
    {
        const QPalette::ColorGroup group =
                                          option.state & QStyle::State_Enabled ?
                                          QPalette::Normal : QPalette::Disabled;

        QLinearGradient gradient(option.rect.topLeft(),
                                 option.rect.bottomRight());
        gradient.setColorAt(0,
                            option.palette.color(group,
                                                 QPalette::Highlight).light());
        gradient.setColorAt(1, Qt::transparent);

        painter->fillRect(option.rect, gradient);
    }

    QFont painterFont = painter->font();
    painterFont.setWeight(QFont::Bold);
    QFontMetrics metrics(painterFont);
    painter->setFont(painterFont);

    QPainterPath path;
    path.addRect(option.rect.left(),
                 option.rect.bottom() - 2,
                 option.rect.width(),
                 2);

    QLinearGradient gradient(option.rect.topLeft(),
                             option.rect.bottomRight());
    gradient.setColorAt(0, color);
    gradient.setColorAt(1, Qt::transparent);

    painter->setBrush(gradient);
    painter->fillPath(path, gradient);

    opt.rect.setLeft(opt.rect.left() + (iconSize / 4));
    starRect.setLeft(starRect.left() + (iconSize / 4));
    starRect.setRight(starRect.right() + (iconSize / 4));

    bool paintIcon = true;
    bool paintText = true;

    QPixmap icon;
    switch (sortRole) {
        case DolphinView::SortByName:
            paintIcon = false;
            break;

        case DolphinView::SortByDate:
            paintIcon = false;
            break;

        case DolphinView::SortByPermissions:
            paintIcon = false; // FIXME: let's think about how to represent permissions
            break;

        case DolphinView::SortByOwner: {
            opt.rect.setTop(option.rect.top() + (iconSize / 4));
            KUser user(category);
            if (QFile::exists(user.homeDir() + QDir::separator() + ".face.icon"))
            {
                icon = QPixmap::fromImage(QImage(user.homeDir() + QDir::separator() + ".face.icon")).scaled(iconSize, iconSize);
            }
            else
            {
                icon = KIconLoader::global()->loadIcon("user", K3Icon::Small);
            }
            break;
        }

        case DolphinView::SortByGroup:
            paintIcon = false;
            break;

        case DolphinView::SortBySize:
            paintIcon = false;
            break;

        case DolphinView::SortByType: {
            opt.rect.setTop(option.rect.top() + (option.rect.height() / 2) - (iconSize / 2));
            const KDirModel *model = static_cast<const KDirModel*>(index.model());
            KFileItem *item = model->itemForIndex(index);
            icon = KIconLoader::global()->loadIcon(KMimeType::iconNameForUrl(item->url()),
                                                   K3Icon::Small);
            break;
        }

#ifdef HAVE_NEPOMUK
        case DolphinView::SortByRating: {
            paintText = false;
            paintIcon = false;

            starRect.setTop(option.rect.top() + (option.rect.height() / 2) - (iconSize / 2));
            starRect.setSize(QSize(iconSize, iconSize));

            QPixmap pixmap = KIconLoader::global()->loadIcon("rating", K3Icon::Small);
            QPixmap smallPixmap = KIconLoader::global()->loadIcon("rating", K3Icon::NoGroup, iconSize / 2);
            QPixmap disabledPixmap = KIconLoader::global()->loadIcon("rating", K3Icon::Small);

            KPixmapEffect::toGray(disabledPixmap, false);

            int rating = category.toInt();

            for (int i = 0; i < rating - (rating % 2); i += 2) {
                painter->drawPixmap(starRect, pixmap);
                starRect.setLeft(starRect.left() + iconSize + (iconSize / 4) /* separator between stars */);
            }

            if (rating && rating % 2) {
                starRect.setTop(option.rect.top() + (option.rect.height() / 2) - (iconSize / 4));
                starRect.setSize(QSize(iconSize / 2, iconSize / 2));
                painter->drawPixmap(starRect, smallPixmap);
                starRect.setTop(opt.rect.top() + (option.rect.height() / 2) - (iconSize / 2));
                starRect.setSize(QSize(iconSize / 2, iconSize / 2));
                starRect.setLeft(starRect.left() + (iconSize / 2) + (iconSize / 4));
                starRect.setSize(QSize(iconSize, iconSize));
            }

            for (int i = rating; i < 9; i += 2) {
                painter->drawPixmap(starRect, disabledPixmap);
                starRect.setLeft(starRect.left() + iconSize + (iconSize / 4));
            }

            break;
        }

        case DolphinView::SortByTags:
            paintIcon = false;
            break;
#endif
    }

    if (paintIcon) {
        painter->drawPixmap(QRect(opt.rect.left(), opt.rect.top(), iconSize, iconSize), icon);
        opt.rect.setLeft(opt.rect.left() + iconSize + (iconSize / 4));
    }

    if (paintText) {
        opt.rect.setTop(option.rect.top() + (iconSize / 4));
        opt.rect.setBottom(opt.rect.bottom() - 2);
        painter->setPen(color);

        painter->drawText(opt.rect, Qt::AlignVCenter | Qt::AlignLeft,
        metrics.elidedText(category, Qt::ElideRight, opt.rect.width()));
    }

    painter->restore();
}

int DolphinItemCategorizer::categoryHeight(const QStyleOption &option) const
{
    int iconSize = KIconLoader::global()->currentSize(K3Icon::Small);

    return qMax(option.fontMetrics.height() + (iconSize / 4) * 2 + 2, iconSize + (iconSize / 4) * 2 + 2) /* 2 gradient */;
}
