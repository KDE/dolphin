/* This file is part of the KDE project
 * Copyright (c) 2012 Dawit Alemayehu <adawit@kde.org>
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

#include "dolphinpart_ext.h"

#include "dolphinpart.h"
#include "views/dolphinview.h"

#include <QVariant>

#include <KFileItemList>

DolphinPartListingFilterExtension::DolphinPartListingFilterExtension(DolphinPart* part)
    : KParts::ListingFilterExtension(part)
    , m_part(part)
{
}

KParts::ListingFilterExtension::FilterModes DolphinPartListingFilterExtension::supportedFilterModes() const
{
    return (KParts::ListingFilterExtension::MimeType |
            KParts::ListingFilterExtension::SubString |
            KParts::ListingFilterExtension::WildCard);
}

bool DolphinPartListingFilterExtension::supportsMultipleFilters(KParts::ListingFilterExtension::FilterMode mode) const
{
    if (mode == KParts::ListingFilterExtension::MimeType)
        return true;

    return false;
}

QVariant DolphinPartListingFilterExtension::filter(KParts::ListingFilterExtension::FilterMode mode) const
{
    QVariant result;

    switch (mode) {
    case KParts::ListingFilterExtension::MimeType:
        result = m_part->view()->mimeTypeFilters();
        break;
    case KParts::ListingFilterExtension::SubString:
    case KParts::ListingFilterExtension::WildCard:
        result = m_part->view()->nameFilter();
        break;
    default:
        break;
    }

    return result;
}

void DolphinPartListingFilterExtension::setFilter(KParts::ListingFilterExtension::FilterMode mode, const QVariant& filter)
{
    switch (mode) {
    case KParts::ListingFilterExtension::MimeType:
        m_part->view()->setMimeTypeFilters(filter.toStringList());
        break;
    case KParts::ListingFilterExtension::SubString:
    case KParts::ListingFilterExtension::WildCard:
        m_part->view()->setNameFilter(filter.toString());
        break;
    default:
        break;
    }
}

////

DolphinPartListingNotificationExtension::DolphinPartListingNotificationExtension(DolphinPart* part)
    : KParts::ListingNotificationExtension(part)
{
}

KParts::ListingNotificationExtension::NotificationEventTypes DolphinPartListingNotificationExtension::supportedNotificationEventTypes() const
{
    return (KParts::ListingNotificationExtension::ItemsAdded |
            KParts::ListingNotificationExtension::ItemsDeleted);
}

void DolphinPartListingNotificationExtension::slotNewItems(const KFileItemList& items)
{
    emit listingEvent(KParts::ListingNotificationExtension::ItemsAdded, items);
}

void DolphinPartListingNotificationExtension::slotItemsDeleted(const KFileItemList& items)
{
    emit listingEvent(KParts::ListingNotificationExtension::ItemsDeleted, items);
}

#include "dolphinpart_ext.moc"
