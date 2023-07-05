/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemmodelbase.h"

KItemModelBase::KItemModelBase(QObject *parent)
    : QObject(parent)
    , m_groupedSorting(false)
    , m_sortRole()
    , m_sortOrder(Qt::AscendingOrder)
{
}

KItemModelBase::KItemModelBase(const QByteArray &sortRole, QObject *parent)
    : QObject(parent)
    , m_groupedSorting(false)
    , m_sortRole(sortRole)
    , m_sortOrder(Qt::AscendingOrder)
{
}

KItemModelBase::~KItemModelBase()
{
}

bool KItemModelBase::setData(int index, const QHash<QByteArray, QVariant> &values)
{
    Q_UNUSED(index)
    Q_UNUSED(values)
    return false;
}

void KItemModelBase::setGroupedSorting(bool grouped)
{
    if (m_groupedSorting != grouped) {
        m_groupedSorting = grouped;
        onGroupedSortingChanged(grouped);
        Q_EMIT groupedSortingChanged(grouped);
    }
}

bool KItemModelBase::groupedSorting() const
{
    return m_groupedSorting;
}

void KItemModelBase::setSortRole(const QByteArray &role, bool resortItems)
{
    if (role != m_sortRole) {
        const QByteArray previous = m_sortRole;
        m_sortRole = role;
        onSortRoleChanged(role, previous, resortItems);
        Q_EMIT sortRoleChanged(role, previous);
    }
}

QByteArray KItemModelBase::sortRole() const
{
    return m_sortRole;
}

void KItemModelBase::setSortOrder(Qt::SortOrder order)
{
    if (order != m_sortOrder) {
        const Qt::SortOrder previous = m_sortOrder;
        m_sortOrder = order;
        onSortOrderChanged(order, previous);
        Q_EMIT sortOrderChanged(order, previous);
    }
}

QString KItemModelBase::roleDescription(const QByteArray &role) const
{
    return role;
}

QList<QPair<int, QVariant>> KItemModelBase::groups() const
{
    return QList<QPair<int, QVariant>>();
}

bool KItemModelBase::setExpanded(int index, bool expanded)
{
    Q_UNUSED(index)
    Q_UNUSED(expanded)
    return false;
}

bool KItemModelBase::isExpanded(int index) const
{
    Q_UNUSED(index)
    return false;
}

bool KItemModelBase::isExpandable(int index) const
{
    Q_UNUSED(index)
    return false;
}

int KItemModelBase::expandedParentsCount(int index) const
{
    Q_UNUSED(index)
    return 0;
}

QMimeData *KItemModelBase::createMimeData(const KItemSet &indexes) const
{
    Q_UNUSED(indexes)
    return nullptr;
}

int KItemModelBase::indexForKeyboardSearch(const QString &text, int startFromIndex) const
{
    Q_UNUSED(text)
    Q_UNUSED(startFromIndex)
    return -1;
}

bool KItemModelBase::supportsDropping(int index) const
{
    Q_UNUSED(index)
    return false;
}

QString KItemModelBase::blacklistItemDropEventMimeType() const
{
    return QStringLiteral("application/x-dolphin-blacklist-drop");
}

void KItemModelBase::onGroupedSortingChanged(bool current)
{
    Q_UNUSED(current)
}

void KItemModelBase::onSortRoleChanged(const QByteArray &current, const QByteArray &previous, bool resortItems)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    Q_UNUSED(resortItems)
}

void KItemModelBase::onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

QUrl KItemModelBase::url(int index) const
{
    return data(index).value("url").toUrl();
}

bool KItemModelBase::isDir(int index) const
{
    return data(index).value("isDir").toBool();
}

QUrl KItemModelBase::directory() const
{
    return QUrl();
}

#include "moc_kitemmodelbase.cpp"
