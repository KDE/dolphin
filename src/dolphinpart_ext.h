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

#include <kparts/browserextension.h>
#include <kparts/fileinfoextension.h>
#include <kparts/listingfilterextension.h>
#include <kparts/listingnotificationextension.h>
#include <QUrl>

class DolphinPart;

class DolphinPartBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    explicit DolphinPartBrowserExtension( DolphinPart* part );
    void restoreState(QDataStream &stream) override;
    void saveState(QDataStream &stream) override;

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void pasteTo(const QUrl &);
    void reparseConfiguration();

private:
    DolphinPart* m_part;
};

class DolphinPartFileInfoExtension : public KParts::FileInfoExtension
{
    Q_OBJECT

public:
    explicit DolphinPartFileInfoExtension(DolphinPart* part);

    QueryModes supportedQueryModes() const override;
    bool hasSelection() const override;

    KFileItemList queryFor(QueryMode mode) const override;

private:
    DolphinPart* m_part;
};

class DolphinPartListingFilterExtension : public KParts::ListingFilterExtension
{
    Q_OBJECT

public:
    explicit DolphinPartListingFilterExtension(DolphinPart* part);
    FilterModes supportedFilterModes() const override;
    bool supportsMultipleFilters(FilterMode mode) const override;
    QVariant filter(FilterMode mode) const override;
    void setFilter(FilterMode mode, const QVariant& filter) override;

private:
    DolphinPart* m_part;
};

class DolphinPartListingNotificationExtension : public KParts::ListingNotificationExtension
{
    Q_OBJECT

public:
    explicit DolphinPartListingNotificationExtension(DolphinPart* part);
    NotificationEventTypes supportedNotificationEventTypes() const override;

public Q_SLOTS:
    void slotNewItems(const KFileItemList&);
    void slotItemsDeleted(const KFileItemList&);
};

#endif
