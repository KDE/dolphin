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
    KSortFilterProxyModel(parent),
    m_sorting(DolphinView::SortByName),
    m_sortOrder(Qt::AscendingOrder)
{
    setDynamicSortFilter(true);

    // sort by the user visible string for now
    setSortRole(DolphinView::SortByName);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    sort(KDirModel::Name, Qt::AscendingOrder);
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
    KSortFilterProxyModel::sort(column, sortOrder);
}

bool DolphinSortFilterProxyModel::hasChildren(const QModelIndex& parent) const
{
    const QModelIndex sourceParent = mapToSource(parent);
    return sourceModel()->hasChildren(sourceParent);
}

bool DolphinSortFilterProxyModel::canFetchMore(const QModelIndex& parent) const
{
    const QModelIndex sourceParent = mapToSource(parent);
    return sourceModel()->canFetchMore(sourceParent);
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
        KDateTime leftTime, rightTime;
        leftTime.setTime_t(leftFileItem->time(KIO::UDS_MODIFICATION_TIME));
        rightTime.setTime_t(rightFileItem->time(KIO::UDS_MODIFICATION_TIME));
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
    KDirModel* dirModel = static_cast<KDirModel*>(sourceModel());

    const KFileItem* leftFileItem  = dirModel->itemForIndex(left);
    const KFileItem* rightFileItem = dirModel->itemForIndex(right);

    // If we are sorting by rating, folders and files are citizens of the same
    // class. Same if we are sorting by tags.
    if ((sortRole() != DolphinView::SortByRating) &&
        (sortRole() != DolphinView::SortByTags))
    {
        // On our priority, folders go above regular files.
        if (leftFileItem->isDir() && !rightFileItem->isDir()) {
            return true;
        } else if (!leftFileItem->isDir() && rightFileItem->isDir()) {
            return false;
        }
    }

    // Hidden elements go before visible ones, if they both are
    // folders or files.
    if (leftFileItem->isHidden() && !rightFileItem->isHidden()) {
        return true;
    } else if (!leftFileItem->isHidden() && rightFileItem->isHidden()) {
        return false;
    }

    switch (sortRole()) {
    case DolphinView::SortByName: {
        // So we are in the same priority, what counts now is their names.
        const QVariant leftData  = dirModel->data(left, KDirModel::Name);
        const QVariant rightData = dirModel->data(right, KDirModel::Name);
        const QString leftValueString(leftData.toString());
        const QString rightValueString(rightData.toString());

        return sortCaseSensitivity() ?
               (naturalCompare(leftValueString, rightValueString) < 0) :
               (naturalCompare(leftValueString.toLower(), rightValueString.toLower()) < 0);
    }

    case DolphinView::SortBySize: {
        // If we have two folders, what we have to measure is the number of
        // items that contains each other
        if (leftFileItem->isDir() && rightFileItem->isDir()) {
            QVariant leftValue = dirModel->data(left, KDirModel::ChildCountRole);
            int leftCount = leftValue.type() == QVariant::Int ? leftValue.toInt() : KDirModel::ChildCountUnknown;

            QVariant rightValue = dirModel->data(right, KDirModel::ChildCountRole);
            int rightCount = rightValue.type() == QVariant::Int ? rightValue.toInt() : KDirModel::ChildCountUnknown;

            // In the case they two have the same child items, we sort them by
            // their names. So we have always everything ordered. We also check
            // if we are taking in count their cases.
            if (leftCount == rightCount) {
                return sortCaseSensitivity() ? (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                        (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
            }

            // If they had different number of items, we sort them depending
            // on how many items had each other.
            return leftCount < rightCount;
        }

        // If what we are measuring is two files and they have the same size,
        // sort them by their file names.
        if (leftFileItem->size() == rightFileItem->size()) {
            return sortCaseSensitivity() ? (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                    (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
        }

        // If their sizes are different, sort them by their sizes, as expected.
        return leftFileItem->size() < rightFileItem->size();
    }

    case DolphinView::SortByDate: {
        KDateTime leftTime, rightTime;
        leftTime.setTime_t(leftFileItem->time(KIO::UDS_MODIFICATION_TIME));
        rightTime.setTime_t(rightFileItem->time(KIO::UDS_MODIFICATION_TIME));

        if (leftTime == rightTime) {
            return sortCaseSensitivity() ?
                   (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                   (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
        }

        return leftTime > rightTime;
    }

    case DolphinView::SortByPermissions: {
        if (leftFileItem->permissionsString() == rightFileItem->permissionsString()) {
            return sortCaseSensitivity() ?
                   (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                   (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
        }

        return naturalCompare(leftFileItem->permissionsString(),
                              rightFileItem->permissionsString()) < 0;
    }

    case DolphinView::SortByOwner: {
        if (leftFileItem->user() == rightFileItem->user()) {
            return sortCaseSensitivity() ?
                   (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                   (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
        }

        return naturalCompare(leftFileItem->user(), rightFileItem->user()) < 0;
    }

    case DolphinView::SortByGroup: {
        if (leftFileItem->group() == rightFileItem->group()) {
            return sortCaseSensitivity() ? (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                    (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
        }

        return naturalCompare(leftFileItem->group(),
                              rightFileItem->group()) < 0;
    }

    case DolphinView::SortByType: {
        if (leftFileItem->mimetype() == rightFileItem->mimetype()) {
            return sortCaseSensitivity() ?
                   (naturalCompare(leftFileItem->name(), rightFileItem->name()) < 0) :
                   (naturalCompare(leftFileItem->name().toLower(), rightFileItem->name().toLower()) < 0);
        }

        return naturalCompare(leftFileItem->mimeComment(),
                              rightFileItem->mimeComment()) < 0;
    }

#ifdef HAVE_NEPOMUK
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
#endif
    }

    // We have set a SortRole and trust the ProxyModel to do
    // the right thing for now.
    return QSortFilterProxyModel::lessThan(left, right);
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

        if (!tagsString.isEmpty())
            tagsString.resize(tagsString.size() - 2);
    }

    return tagsString;
#else
    return QString();
#endif
}

int DolphinSortFilterProxyModel::naturalCompare(const QString& a,
                                                const QString& b)
{
    // This method chops the input a and b into pieces of
    // digits and non-digits (a1.05 becomes a | 1 | . | 05)
    // and compares these pieces of a and b to each other
    // (first with first, second with second, ...).
    //
    // This is based on the natural sort order code code by Martin Pool
    // http://sourcefrog.net/projects/natsort/
    // Martin Pool agreed to license this under LGPL or GPL.

    const QChar* currA = a.unicode(); // iterator over a
    const QChar* currB = b.unicode(); // iterator over b

    if (currA == currB) {
        return 0;
    }

    const QChar* begSeqA = currA; // beginning of a new character sequence of a
    const QChar* begSeqB = currB;

    while (!currA->isNull() && !currB->isNull()) {
        // find sequence of characters ending at the first non-character
        while (!currA->isNull() && !currA->isDigit()) {
            ++currA;
        }

        while (!currB->isNull() && !currB->isDigit()) {
            ++currB;
        }

        // compare these sequences
        const QString subA(begSeqA, currA - begSeqA);
        const QString subB(begSeqB, currB - begSeqB);
        const int cmp = QString::localeAwareCompare(subA, subB);
        if (cmp != 0) {
            return cmp;
        }

        if (currA->isNull() || currB->isNull()) {
            break;
        }

        // now some digits follow...
        if ((*currA == '0') || (*currB == '0')) {
            // one digit-sequence starts with 0 -> assume we are in a fraction part
            // do left aligned comparison (numbers are considered left aligned)
            while (1) {
                if (!currA->isDigit() && !currB->isDigit()) {
                    break;
                } else if (!currA->isDigit()) {
                    return -1;
                } else if (!currB->isDigit()) {
                    return + 1;
                } else if (*currA < *currB) {
                    return -1;
                } else if (*currA > *currB) {
                    return + 1;
                }
                ++currA;
                ++currB;
            }
        } else {
            // No digit-sequence starts with 0 -> assume we are looking at some integer
            // do right aligned comparison.
            //
            // The longest run of digits wins. That aside, the greatest
            // value wins, but we can't know that it will until we've scanned
            // both numbers to know that they have the same magnitude.

            int weight = 0;
            while (1) {
                if (!currA->isDigit() && !currB->isDigit()) {
                    if (weight != 0) {
                        return weight;
                    }
                    break;
                } else if (!currA->isDigit()) {
                    return -1;
                } else if (!currB->isDigit()) {
                    return + 1;
                } else if ((*currA < *currB) && (weight == 0)) {
                    weight = -1;
                } else if ((*currA > *currB) && (weight == 0)) {
                    weight = + 1;
                }
                ++currA;
                ++currB;
            }
        }

        begSeqA = currA;
        begSeqB = currB;
    }

    if (currA->isNull() && currB->isNull()) {
        return 0;
    }

    return currA->isNull() ? -1 : + 1;
}

#include "dolphinsortfilterproxymodel.moc"
