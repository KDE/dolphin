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
    DolphinPartBrowserExtension( DolphinPart* part );
    virtual void restoreState(QDataStream &stream) Q_DECL_OVERRIDE;
    virtual void saveState(QDataStream &stream) Q_DECL_OVERRIDE;

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
    DolphinPartFileInfoExtension(DolphinPart* part);

    virtual QueryModes supportedQueryModes() const Q_DECL_OVERRIDE;
    virtual bool hasSelection() const Q_DECL_OVERRIDE;

    virtual KFileItemList queryFor(QueryMode mode) const Q_DECL_OVERRIDE;

private:
    DolphinPart* m_part;
};

class DolphinPartListingFilterExtension : public KParts::ListingFilterExtension
{
    Q_OBJECT

public:
    DolphinPartListingFilterExtension(DolphinPart* part);
    virtual FilterModes supportedFilterModes() const Q_DECL_OVERRIDE;
    virtual bool supportsMultipleFilters(FilterMode mode) const Q_DECL_OVERRIDE;
    virtual QVariant filter(FilterMode mode) const Q_DECL_OVERRIDE;
    virtual void setFilter(FilterMode mode, const QVariant& filter) Q_DECL_OVERRIDE;

private:
    DolphinPart* m_part;
};

class DolphinPartListingNotificationExtension : public KParts::ListingNotificationExtension
{
    Q_OBJECT

public:
    DolphinPartListingNotificationExtension(DolphinPart* part);
    virtual NotificationEventTypes supportedNotificationEventTypes() const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void slotNewItems(const KFileItemList&);
    void slotItemsDeleted(const KFileItemList&);
};

#endif
