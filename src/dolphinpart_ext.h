/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2012 Dawit Alemayehu <adawit@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    explicit DolphinPartBrowserExtension(DolphinPart *part);
    void restoreState(QDataStream &stream) override;
    void saveState(QDataStream &stream) override;

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void pasteTo(const QUrl &);
    void reparseConfiguration();

private:
    DolphinPart *m_part;
};

class DolphinPartFileInfoExtension : public KParts::FileInfoExtension
{
    Q_OBJECT

public:
    explicit DolphinPartFileInfoExtension(DolphinPart *part);

    QueryModes supportedQueryModes() const override;
    bool hasSelection() const override;

    KFileItemList queryFor(QueryMode mode) const override;

private:
    DolphinPart *m_part;
};

class DolphinPartListingFilterExtension : public KParts::ListingFilterExtension
{
    Q_OBJECT

public:
    explicit DolphinPartListingFilterExtension(DolphinPart *part);
    FilterModes supportedFilterModes() const override;
    bool supportsMultipleFilters(FilterMode mode) const override;
    QVariant filter(FilterMode mode) const override;
    void setFilter(FilterMode mode, const QVariant &filter) override;

private:
    DolphinPart *m_part;
};

class DolphinPartListingNotificationExtension : public KParts::ListingNotificationExtension
{
    Q_OBJECT

public:
    explicit DolphinPartListingNotificationExtension(DolphinPart *part);
    NotificationEventTypes supportedNotificationEventTypes() const override;

public Q_SLOTS:
    void slotNewItems(const KFileItemList &);
    void slotItemsDeleted(const KFileItemList &);
};

#endif
