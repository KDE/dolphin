/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Dominic Battre <dominic@battre.de>              *
 *   Copyright (C) 2006 by Martin Pool <mbp@canonical.com>                 *
 *   Copyright (C) 2007 by Rafael Fernández López <ereslibre@gmail.com>    *
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

#include "dolphinsortfilterproxymodel.h"

#ifdef HAVE_NEPOMUK
#include <config-nepomuk.h>
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#include <nepomuk/tag.h>
#endif

#include <kdirmodel.h>
#include <kfileitem.h>
#include <kdatetime.h>
#include <klocale.h>

static DolphinView::Sorting sortingTypeTable[] =
{
    DolphinView::SortByName,        // KDirModel::Name
    DolphinView::SortBySize,        // KDirModel::Size
    DolphinView::SortByDate,        // KDirModel::ModifiedTime
    DolphinView::SortByPermissions, // KDirModel::Permissions
    DolphinView::SortByOwner,       // KDirModel::Owner
    DolphinView::SortByGroup,       // KDirModel::Group
    DolphinView::SortByType         // KDirModel::Type
#ifdef HAVE_NEPOMUK
    , DolphinView::SortByRating
    , DolphinView::SortByTags
#endif
};

DolphinSortFilterProxyModel::DolphinSortFilterProxyModel(QObject* parent) :
    KDirSortFilterProxyModel(parent),
    m_sorting(DolphinView::SortByName),
    m_sortOrder(Qt::AscendingOrder)
{
}

DolphinSortFilterProxyModel::~DolphinSortFilterProxyModel()
{
}

void DolphinSortFilterProxyModel::setSorting(DolphinView::Sorting sorting)
{
    // change the sorting column by keeping the current sort order
    sort(sorting, m_sortOrder);
}

void DolphinSortFilterProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    // change the sort order by keeping the current column
    sort(m_sorting, sortOrder);
}

void DolphinSortFilterProxyModel::sort(int column, Qt::SortOrder sortOrder)
{
    m_sorting = sortingForColumn(column);
    m_sortOrder = sortOrder;
    setSortRole(m_sorting);
    KDirSortFilterProxyModel::sort(column, sortOrder);
    emit sortingRoleChanged();
}

DolphinView::Sorting DolphinSortFilterProxyModel::sortingForColumn(int column)
{
    Q_ASSERT(column >= 0);
    Q_ASSERT(column < static_cast<int>(sizeof(sortingTypeTable) / sizeof(DolphinView::Sorting)));
    return sortingTypeTable[column];
}

bool DolphinSortFilterProxyModel::lessThanGeneralPurpose(const QModelIndex &left,
                                                         const QModelIndex &right) const
{
    KDirModel* dirModel = static_cast<KDirModel*>(sourceModel());

    const KFileItem* leftFileItem  = dirModel->itemForIndex(left);
    const KFileItem* rightFileItem = dirModel->itemForIndex(right);

    //FIXME left.column() should be used instead!
    switch (sortRole()) {
    case DolphinView::SortByName: {
        QString leftFileName(leftFileItem->name());
        if (leftFileName.at(0) == '.') {
            leftFileName = leftFileName.mid(1);
        }

        QString rightFileName(rightFileItem->name());
        if (rightFileName.at(0) == '.') {
            rightFileName = rightFileName.mid(1);
        }

        // We don't care about case for building categories. We also don't
        // want here to compare by a natural comparison.
        return naturalCompare(leftFileName, rightFileName) < 0;
    }

    case DolphinView::SortBySize:
        // If we are sorting by size, show folders first. We will sort them
        // correctly later.
        return leftFileItem->isDir() && !rightFileItem->isDir();

    case DolphinView::SortByDate: {
        KDateTime leftTime = leftFileItem->time(KFileItem::ModificationTime);
        KDateTime rightTime = rightFileItem->time(KFileItem::ModificationTime);
        return leftTime > rightTime;
    }

    case DolphinView::SortByPermissions: {
        return naturalCompare(leftFileItem->permissionsString(),
                            rightFileItem->permissionsString()) < 0;
    }

    case DolphinView::SortByOwner: {
        return naturalCompare(leftFileItem->user().toLower(),
                              rightFileItem->user().toLower()) < 0;
    }

    case DolphinView::SortByGroup: {
        return naturalCompare(leftFileItem->group().toLower(),
                              rightFileItem->group().toLower()) < 0;
    }

    case DolphinView::SortByType: {
        // If we are sorting by size, show folders first. We will sort them
        // correctly later.
        if (leftFileItem->isDir() && !rightFileItem->isDir()) {
            return true;
        } else if (!leftFileItem->isDir() && rightFileItem->isDir()) {
            return false;
        }

        return naturalCompare(leftFileItem->mimeComment().toLower(),
                              rightFileItem->mimeComment().toLower()) < 0;
    }
#ifdef HAVE_NEPOMUK
    case DolphinView::SortByRating: {
        const quint32 leftRating = ratingForIndex(left);
        const quint32 rightRating = ratingForIndex(right);
        return leftRating > rightRating;
    }
    case DolphinView::SortByTags: {
        const QString leftTags = tagsForIndex(left);
        const QString rightTags = tagsForIndex(right);

        if (leftTags.isEmpty() && !rightTags.isEmpty())
            return false;
        else if (!leftTags.isEmpty() && rightTags.isEmpty())
            return true;

        return naturalCompare(tagsForIndex(left), tagsForIndex(right)) < 0;
    }
#endif
    default:
        break;
    }
    return false;
}

bool DolphinSortFilterProxyModel::lessThan(const QModelIndex& left,
                                           const QModelIndex& right) const
{
#ifdef HAVE_NEPOMUK
    KDirModel* dirModel = static_cast<KDirModel*>(sourceModel());

    const KFileItem* leftFileItem  = dirModel->itemForIndex(left);
    const KFileItem* rightFileItem = dirModel->itemForIndex(right);


    // Hidden elements go before visible ones, if they both are
    // folders or files.
    if (leftFileItem->isHidden() && !rightFileItem->isHidden()) {
        return true;
    } else if (!leftFileItem->isHidden() && rightFileItem->isHidden()) {
        return false;
    }

    //FIXME left.column() should be used instead!
    switch (sortRole()) {
    case DolphinView::SortByRating: {
        const quint32 leftRating  = ratingForIndex(left);
        const quint32 rightRating = ratingForIndex(right);

        if (leftRating == rightRating) {
            // On our priority, folders go above regular files.
            // This checks are needed (don't think it's the same doing it here
            // than above). Here we make dirs citizens of first class because
            // we know we are on the same category. On the check we do on the
            // top of the method we don't know, so we remove that check when we
            // are sorting by rating. (ereslibre)
            if (leftFileItem->isDir() && !rightFileItem->isDir()) {
                return true;
            } else if (!leftFileItem->isDir() && rightFileItem->isDir()) {
                return false;
            }

            return sortCaseSensitivity() ?
                   (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                   (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
        }

        return leftRating > rightRating;
    }

    case DolphinView::SortByTags: {
        const QString leftTags = tagsForIndex(left);
        const QString rightTags = tagsForIndex(right);

        if (leftTags == rightTags) {
            // On our priority, folders go above regular files.
            // This checks are needed (don't think it's the same doing it here
            // than above). Here we make dirs citizens of first class because
            // we know we are on the same category. On the check we do on the
            // top of the method we don't know, so we remove that check when we
            // are sorting by tags. (ereslibre)
            if (leftFileItem->isDir() && !rightFileItem->isDir()) {
                return true;
            } else if (!leftFileItem->isDir() && rightFileItem->isDir()) {
                return false;
            }

            return sortCaseSensitivity() ?
                   (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                   (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
        }

        return naturalCompare(leftTags, rightTags) < 0;
    }
    }
#endif

    return KDirSortFilterProxyModel::lessThan(left, right);
}

quint32 DolphinSortFilterProxyModel::ratingForIndex(const QModelIndex& index)
{
#ifdef HAVE_NEPOMUK
    quint32 rating = 0;

    const KDirModel* dirModel = static_cast<const KDirModel*>(index.model());
    KFileItem* item = dirModel->itemForIndex(index);
    if (item != 0) {
        const Nepomuk::Resource resource(item->url().url(), Nepomuk::NFO::File());
        rating = resource.rating();
    }
    return rating;
#else
    Q_UNUSED(index);
    return 0;
#endif
}

QString DolphinSortFilterProxyModel::tagsForIndex(const QModelIndex& index)
{
#ifdef HAVE_NEPOMUK
    QString tagsString;

    const KDirModel* dirModel = static_cast<const KDirModel*>(index.model());
    KFileItem* item = dirModel->itemForIndex(index);
    if (item != 0) {
        const Nepomuk::Resource resource(item->url().url(), Nepomuk::NFO::File());
        const QList<Nepomuk::Tag> tags = resource.tags();
        QStringList stringList;
        foreach (const Nepomuk::Tag& tag, tags) {
            stringList.append(tag.label());
        }
        stringList.sort();

        foreach (const QString& str, stringList) {
            tagsString += str;
            tagsString += ", ";
        }

        if (!tagsString.isEmpty()) {
            tagsString.resize(tagsString.size() - 2);
        }
    }

    return tagsString;
#else
    Q_UNUSED(index);
    return QString();
#endif
}

#include "dolphinsortfilterproxymodel.moc"
