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

#ifndef DOLPHINPART_EXT_H
#define DOLPHINPART_EXT_H


#include <kparts/listingextension.h>

class DolphinPart;

class DolphinPartListingFilterExtension : public KParts::ListingFilterExtension
{
    Q_OBJECT

public:
    DolphinPartListingFilterExtension(DolphinPart* part);
    virtual FilterModes supportedFilterModes() const;
    virtual bool supportsMultipleFilters(FilterMode mode) const;
    virtual QVariant filter(FilterMode mode) const;
    virtual void setFilter(FilterMode mode, const QVariant& filter);

private:
    DolphinPart* m_part;
};

class DolphinPartListingNotificationExtension : public KParts::ListingNotificationExtension
{
    Q_OBJECT

public:
    DolphinPartListingNotificationExtension(DolphinPart* part);
    virtual NotificationEventTypes supportedNotificationEventTypes() const;

public Q_SLOTS:
    void slotNewItems(const KFileItemList&);
    void slotItemsDeleted(const KFileItemList&);
};

#endif
