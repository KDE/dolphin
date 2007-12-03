/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Dominic Battre <dominic@battre.de>              *
 *   Copyright (C) 2006 by Martin Pool <mbp@canonical.com>                 *
 *   Copyright (C) 2007 by Rafael Fernández López <ereslibre@kde.org>    *
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

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#include <nepomuk/tag.h>
#endif

#include "dolphinmodel.h"

#include <kfileitem.h>
#include <kdatetime.h>
#include <klocale.h>

static DolphinView::Sorting sortingTypeTable[] =
{
    DolphinView::SortByName,        // DolphinModel::Name
    DolphinView::SortBySize,        // DolphinModel::Size
    DolphinView::SortByDate,        // DolphinModel::ModifiedTime
    DolphinView::SortByPermissions, // DolphinModel::Permissions
    DolphinView::SortByOwner,       // DolphinModel::Owner
    DolphinView::SortByGroup,       // DolphinModel::Group
    DolphinView::SortByType         // DolphinModel::Type
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
    m_sorting = sorting;

    // change the sorting column by keeping the current sort order
    KDirSortFilterProxyModel::sort((int) m_sorting, m_sortOrder);
}

void DolphinSortFilterProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    m_sortOrder = sortOrder;

    // change the sort order by keeping the current column
    KDirSortFilterProxyModel::sort((int) m_sorting, m_sortOrder);
}

void DolphinSortFilterProxyModel::sort(int column, Qt::SortOrder sortOrder)
{
    m_sorting = sortingForColumn(column);
    m_sortOrder = sortOrder;

    emit sortingRoleChanged();
    KDirSortFilterProxyModel::sort((int) m_sorting, sortOrder);
}

DolphinView::Sorting DolphinSortFilterProxyModel::sortingForColumn(int column)
{
    Q_ASSERT(column >= 0);
    Q_ASSERT(column < static_cast<int>(sizeof(sortingTypeTable) / sizeof(DolphinView::Sorting)));
    return sortingTypeTable[column];
}

int DolphinSortFilterProxyModel::compareCategories(const QModelIndex &left,
                                                   const QModelIndex &right) const
{
#ifdef HAVE_NEPOMUK
    DolphinModel* dolphinModel = static_cast<DolphinModel*>(sourceModel());

    const KFileItem leftFileItem  = dolphinModel->itemForIndex(left);
    const KFileItem rightFileItem = dolphinModel->itemForIndex(right);

    switch (left.column()) {

    case DolphinView::SortByRating: {
        const quint32 leftRating = DolphinModel::ratingForIndex(left);
        const quint32 rightRating = DolphinModel::ratingForIndex(right);
        return leftRating > rightRating;
    }

    case DolphinView::SortByTags: {
        const QString leftTags = DolphinModel::tagsForIndex(left);
        const QString rightTags = DolphinModel::tagsForIndex(right);

        if (leftTags.isEmpty() && !rightTags.isEmpty())
            return false;
        else if (!leftTags.isEmpty() && rightTags.isEmpty())
            return true;

        return naturalCompare(DolphinModel::tagsForIndex(left), DolphinModel::tagsForIndex(right)) < 0;
    }

    default:
        break;

    }
#endif
    return KDirSortFilterProxyModel::compareCategories(left, right);
}

bool DolphinSortFilterProxyModel::subsortLessThan(const QModelIndex& left,
                                                  const QModelIndex& right) const
{
#ifdef HAVE_NEPOMUK
    DolphinModel* dolphinModel = static_cast<DolphinModel*>(sourceModel());

    const KFileItem leftFileItem  = dolphinModel->itemForIndex(left);
    const KFileItem rightFileItem = dolphinModel->itemForIndex(right);

    // On our priority, folders go above regular files.
    if (leftFileItem.isDir() && !rightFileItem.isDir()) {
        return true;
    } else if (!leftFileItem.isDir() && rightFileItem.isDir()) {
        return false;
    }

    // Hidden elements go before visible ones, if they both are
    // folders or files.
    if (leftFileItem.isHidden() && !rightFileItem.isHidden()) {
        return true;
    } else if (!leftFileItem.isHidden() && rightFileItem.isHidden()) {
        return false;
    }

    switch (left.column()) {

    case DolphinView::SortByRating: {
        const quint32 leftRating  = DolphinModel::ratingForIndex(left);
        const quint32 rightRating = DolphinModel::ratingForIndex(right);

        if (leftRating == rightRating) {
            // On our priority, folders go above regular files.
            // This checks are needed (don't think it's the same doing it here
            // than above). Here we make dirs citizens of first class because
            // we know we are on the same category. On the check we do on the
            // top of the method we don't know, so we remove that check when we
            // are sorting by rating. (ereslibre)
            if (leftFileItem.isDir() && !rightFileItem.isDir()) {
                return true;
            } else if (!leftFileItem.isDir() && rightFileItem.isDir()) {
                return false;
            }

            return sortCaseSensitivity() ?
                   (naturalCompare(leftFileItem.name(), rightFileItem.name()) < 0) :
                   (naturalCompare(leftFileItem.name().toLower(), rightFileItem.name().toLower()) < 0);
        }

        return leftRating > rightRating;
    }

    case DolphinView::SortByTags: {
        const QString leftTags = DolphinModel::tagsForIndex(left);
        const QString rightTags = DolphinModel::tagsForIndex(right);

        if (leftTags == rightTags) {
            // On our priority, folders go above regular files.
            // This checks are needed (don't think it's the same doing it here
            // than above). Here we make dirs citizens of first class because
            // we know we are on the same category. On the check we do on the
            // top of the method we don't know, so we remove that check when we
            // are sorting by tags. (ereslibre)
            if (leftFileItem.isDir() && !rightFileItem.isDir()) {
                return true;
            } else if (!leftFileItem.isDir() && rightFileItem.isDir()) {
                return false;
            }

            return sortCaseSensitivity() ?
                   (naturalCompare(leftFileItem.name(), rightFileItem.name()) < 0) :
                   (naturalCompare(leftFileItem.name().toLower(), rightFileItem.name().toLower()) < 0);
        }

        return naturalCompare(leftTags, rightTags) < 0;
    }

    default:
        break;

    }
#endif
    return KDirSortFilterProxyModel::subsortLessThan(left, right);
}

#include "dolphinsortfilterproxymodel.moc"
