/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
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

#include "dolphinmodel.h"

#include "dolphinsortfilterproxymodel.h"

#include "kcategorizedview.h"

#include <kdatetime.h>
#include <kdirmodel.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kuser.h>
#include <kmimetype.h>
#include <kstandarddirs.h>

#include <QList>
#include <QSortFilterProxyModel>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QDir>
#include <QFileInfo>

const char* const DolphinModel::m_others = I18N_NOOP2("@title:group Name", "Others");

DolphinModel::DolphinModel(QObject* parent) :
    KDirModel(parent),
    m_hasVersionData(false),
    m_revisionHash()
{
}

DolphinModel::~DolphinModel()
{
}

bool DolphinModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if ((index.column() == DolphinModel::Version) && (role == Qt::DecorationRole)) {
        // TODO: remove data again when items are deleted...

        const QPersistentModelIndex key = index;
        const KVersionControlPlugin::VersionState state = static_cast<KVersionControlPlugin::VersionState>(value.toInt());
        if (m_revisionHash.value(key, KVersionControlPlugin::UnversionedVersion) != state) {
            if (!m_hasVersionData) {
                connect(this, SIGNAL(rowsRemoved (const QModelIndex&, int, int)),
                        this, SLOT(slotRowsRemoved(const QModelIndex&, int, int)));
                m_hasVersionData = true;
            }

            m_revisionHash.insert(key, state);
            emit dataChanged(index, index);
            return true;
        }
    }

    return KDirModel::setData(index, value, role);
}

QVariant DolphinModel::data(const QModelIndex& index, int role) const
{
    switch (role) {
    case KCategorizedSortFilterProxyModel::CategoryDisplayRole:
        return displayRoleData(index);

    case KCategorizedSortFilterProxyModel::CategorySortRole:
        return sortRoleData(index);

    case Qt::DecorationRole:
        if (index.column() == DolphinModel::Version) {
            return m_revisionHash.value(index, KVersionControlPlugin::UnversionedVersion);
        }
        break;

    case Qt::DisplayRole:
        if (index.column() == DolphinModel::Version) {
            switch (m_revisionHash.value(index, KVersionControlPlugin::UnversionedVersion)) {
            case KVersionControlPlugin::NormalVersion:
                return i18nc("@item::intable", "Normal");
            case KVersionControlPlugin::UpdateRequiredVersion:
                return i18nc("@item::intable", "Update required");
            case KVersionControlPlugin::LocallyModifiedVersion:
                return i18nc("@item::intable", "Locally modified");
            case KVersionControlPlugin::AddedVersion:
                return i18nc("@item::intable", "Added");
            case KVersionControlPlugin::RemovedVersion:
                return i18nc("@item::intable", "Removed");
            case KVersionControlPlugin::ConflictingVersion:
                return i18nc("@item::intable", "Conflicting");
            case KVersionControlPlugin::UnversionedVersion:
            default:
                return i18nc("@item::intable", "Unversioned");
            }
        }
        break;

    default:
        break;
    }

    return KDirModel::data(index, role);
}

QVariant DolphinModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole)) {
        if (section < KDirModel::ColumnCount) {
            return KDirModel::headerData(section, orientation, role);
        }

        Q_ASSERT(section == DolphinModel::Version);
        return i18nc("@title::column", "Version");
    }
    return QVariant();
}

int DolphinModel::columnCount(const QModelIndex& parent) const
{
    return KDirModel::columnCount(parent) + (ExtraColumnCount - ColumnCount);
}

void DolphinModel::clearVersionData()
{
    m_revisionHash.clear();
    m_hasVersionData = false;
}

bool DolphinModel::hasVersionData() const
{
    return m_hasVersionData;
}

void DolphinModel::slotRowsRemoved(const QModelIndex& parent, int start, int end)
{
    if (m_hasVersionData) {
        const int column = parent.column();
        for (int row = start; row <= end; ++row) {
            m_revisionHash.remove(parent.child(row, column));
        }
    }
}

QVariant DolphinModel::displayRoleData(const QModelIndex& index) const
{
    QString retString;

    if (!index.isValid()) {
        return retString;
    }

    const KDirModel *dirModel = qobject_cast<const KDirModel*>(index.model());
    KFileItem item = dirModel->itemForIndex(index);

    switch (index.column()) {
    case KDirModel::Name: {
        // KDirModel checks columns to know to which role are
        // we talking about
        const QModelIndex nameIndex = index.model()->index(index.row(), KDirModel::Name, index.parent());
        if (!nameIndex.isValid()) {
            return retString;
        }
        const QVariant data = nameIndex.model()->data(nameIndex, Qt::DisplayRole);
        const QString name = data.toString();
        if (!name.isEmpty()) {
            if (!item.isHidden() && name.at(0).isLetter())
                retString = name.at(0).toUpper();
            else if (item.isHidden()) {
                if (name.at(0) == '.') {
                    if (name.size() > 1 && name.at(1).isLetter()) {
                        retString = name.at(1).toUpper();
                    } else {
                        retString = i18nc("@title:group Name", m_others);
                    }
                } else {
                    retString = name.at(0).toUpper();
                }
            } else {
                bool validCategory = false;

                const QString str(name.toUpper());
                const QChar* currA = str.unicode();
                while (!currA->isNull() && !validCategory) {
                    if (currA->isLetter()) {
                        validCategory = true;
                    } else if (currA->isDigit()) {
                        return i18nc("@title:group Name", m_others);
                    } else {
                        ++currA;
                    }
                }

                retString = validCategory ? *currA : i18nc("@title:group Name", m_others);
            }
        }
        break;
    }

    case KDirModel::Size: {
        const KIO::filesize_t fileSize = !item.isNull() ? item.size() : ~0U;
        if (!item.isNull() && item.isDir()) {
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

    case KDirModel::ModifiedTime: {
        KDateTime modifiedTime = item.time(KFileItem::ModificationTime);
        modifiedTime = modifiedTime.toLocalZone();

        const QDate currentDate = KDateTime::currentLocalDateTime().date();
        const QDate modifiedDate = modifiedTime.date();

        const int daysDistance = modifiedDate.daysTo(currentDate);

        int yearForCurrentWeek = 0;
        int currentWeek = currentDate.weekNumber(&yearForCurrentWeek);
        if (yearForCurrentWeek == currentDate.year() + 1) {
            currentWeek = 53;
        }

        int yearForModifiedWeek = 0;
        int modifiedWeek = modifiedDate.weekNumber(&yearForModifiedWeek);
        if (yearForModifiedWeek == modifiedDate.year() + 1) {
            modifiedWeek = 53;
        }

        if (currentDate.year() == modifiedDate.year() && currentDate.month() == modifiedDate.month()) {
            if (modifiedWeek > currentWeek) {
                // use case: modified date = 2010-01-01, current date = 2010-01-22
                //           modified week = 53,         current week = 3
                modifiedWeek = 0;
            }
            switch (currentWeek - modifiedWeek) {
            case 0:
                switch (daysDistance) {
                case 0:  retString = i18nc("@title:group Date", "Today"); break;
                case 1:  retString = i18nc("@title:group Date", "Yesterday"); break;
                default: retString = modifiedTime.toString(i18nc("@title:group The week day name: %A", "%A"));
                }
                break;
            case 1:
                retString = i18nc("@title:group Date", "Last Week");
                break;
            case 2:
                retString = i18nc("@title:group Date", "Two Weeks Ago");
                break;
            case 3:
                retString = i18nc("@title:group Date", "Three Weeks Ago");
                break;
            case 4:
            case 5:
                retString = i18nc("@title:group Date", "Earlier this Month");
                break;
            default:
                Q_ASSERT(false);
            }
        } else {
            const QDate lastMonthDate = currentDate.addMonths(-1);
            if  (lastMonthDate.year() == modifiedDate.year() && lastMonthDate.month() == modifiedDate.month()) {
                if (daysDistance == 1) {
                    retString = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Yesterday (%B, %Y)"));
                } else if (daysDistance <= 7) {
                    retString = modifiedTime.toString(i18nc("@title:group The week day name: %A, %B is full month name in current locale, and %Y is full year number", "%A (%B, %Y)"));
                } else if (daysDistance <= 7 * 2) {
                    retString = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Last Week (%B, %Y)"));
                } else if (daysDistance <= 7 * 3) {
                    retString = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Two Weeks Ago (%B, %Y)"));
                } else if (daysDistance <= 7 * 4) {
                    retString = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Three Weeks Ago (%B, %Y)"));
                } else {
                    retString = modifiedTime.toString(i18nc("@title:group Date: %B is full month name in current locale, and %Y is full year number", "Earlier on %B, %Y"));
                }
            } else {
                retString = modifiedTime.toString(i18nc("@title:group The month and year: %B is full month name in current locale, and %Y is full year number", "%B, %Y"));
            }
        }
        break;
    }

    case KDirModel::Permissions: {
        QString user;
        QString group;
        QString others;

        QFileInfo info(item.url().pathOrUrl());

        // set user string
        if (info.permission(QFile::ReadUser)) {
            user = i18nc("@item:intext Access permission, concatenated", "Read, ");
        }
        if (info.permission(QFile::WriteUser)) {
            user += i18nc("@item:intext Access permission, concatenated", "Write, ");
        }
        if (info.permission(QFile::ExeUser)) {
            user += i18nc("@item:intext Access permission, concatenated", "Execute, ");
        }
        user = user.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : user.mid(0, user.count() - 2);

        // set group string
        if (info.permission(QFile::ReadGroup)) {
            group = i18nc("@item:intext Access permission, concatenated", "Read, ");
        }
        if (info.permission(QFile::WriteGroup)) {
            group += i18nc("@item:intext Access permission, concatenated", "Write, ");
        }
        if (info.permission(QFile::ExeGroup)) {
            group += i18nc("@item:intext Access permission, concatenated", "Execute, ");
        }
        group = group.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : group.mid(0, group.count() - 2);

        // set permission string
        if (info.permission(QFile::ReadOther)) {
            others = i18nc("@item:intext Access permission, concatenated", "Read, ");
        }
        if (info.permission(QFile::WriteOther)) {
            others += i18nc("@item:intext Access permission, concatenated", "Write, ");
        }
        if (info.permission(QFile::ExeOther)) {
            others += i18nc("@item:intext Access permission, concatenated", "Execute, ");
        }
        others = others.isEmpty() ? i18nc("@item:intext Access permission, concatenated", "Forbidden") : others.mid(0, others.count() - 2);

        retString = i18nc("@title:group Files and folders by permissions", "(User: %1) (Group: %2) (Others: %3)", user, group, others);
        break;
    }

    case KDirModel::Owner:
        retString = item.user();
        break;

    case KDirModel::Group:
        retString = item.group();
        break;

    case KDirModel::Type:
        retString = item.mimeComment();
        break;

    case DolphinModel::Version:
        retString = "test";
        break;
    }

    return retString;
}

QVariant DolphinModel::sortRoleData(const QModelIndex& index) const
{
    QVariant retVariant;

    if (!index.isValid()) {
        return retVariant;
    }

    const KDirModel *dirModel = qobject_cast<const KDirModel*>(index.model());
    KFileItem item = dirModel->itemForIndex(index);

    switch (index.column()) {
    case KDirModel::Name: {
        retVariant = data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole);
        if (retVariant == i18nc("@title:group Name", m_others)) {
            // assure that the "Others" group is always the last categorization
            retVariant = QString('Z').append(QChar::ReplacementCharacter);
        }
        break;
    }

    case KDirModel::Size: {
        const KIO::filesize_t fileSize = !item.isNull() ? item.size() : ~0U;
        if (item.isDir()) {
            retVariant = 0;
        } else if (fileSize < 5242880) {
            retVariant = 1;
        } else if (fileSize < 10485760) {
            retVariant = 2;
        } else {
            retVariant = 3;
        }
        break;
    }

    case KDirModel::ModifiedTime: {
        KDateTime modifiedTime = item.time(KFileItem::ModificationTime);
        modifiedTime = modifiedTime.toLocalZone();

        const QDate currentDate = KDateTime::currentLocalDateTime().date();
        const QDate modifiedDate = modifiedTime.date();

        retVariant = -modifiedDate.daysTo(currentDate);
        break;
    }

    case KDirModel::Permissions: {
        QFileInfo info(item.url().pathOrUrl());

        retVariant = -KDirSortFilterProxyModel::pointsForPermissions(info);
        break;
    }

    case KDirModel::Owner:
        retVariant = item.user();
        break;

    case KDirModel::Group:
        retVariant = item.group();
        break;

    case KDirModel::Type:
        if (item.isDir()) {
            // when sorting we want folders to be placed first
            retVariant = QString(); // krazy:exclude=nullstrassign
        } else {
            retVariant = item.mimeComment();
        }
        break;

    default:
        break;
    }

    return retVariant;
}
