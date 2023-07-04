/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2012 Dawit Alemayehu <adawit@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "dolphinpart_ext.h"

#include "dolphinpart.h"
#include "views/dolphinview.h"

DolphinPartBrowserExtension::DolphinPartBrowserExtension(DolphinPart *part)
    : KParts::BrowserExtension(part)
    , m_part(part)
{
}

void DolphinPartBrowserExtension::restoreState(QDataStream &stream)
{
    KParts::BrowserExtension::restoreState(stream);
    m_part->view()->restoreState(stream);
}

void DolphinPartBrowserExtension::saveState(QDataStream &stream)
{
    KParts::BrowserExtension::saveState(stream);
    m_part->view()->saveState(stream);
}

void DolphinPartBrowserExtension::cut()
{
    m_part->view()->cutSelectedItemsToClipboard();
}

void DolphinPartBrowserExtension::copy()
{
    m_part->view()->copySelectedItemsToClipboard();
}

void DolphinPartBrowserExtension::paste()
{
    m_part->view()->paste();
}

void DolphinPartBrowserExtension::pasteTo(const QUrl &)
{
    m_part->view()->pasteIntoFolder();
}

void DolphinPartBrowserExtension::reparseConfiguration()
{
    m_part->view()->readSettings();
}

DolphinPartFileInfoExtension::DolphinPartFileInfoExtension(DolphinPart *part)
    : KParts::FileInfoExtension(part)
    , m_part(part)
{
}

bool DolphinPartFileInfoExtension::hasSelection() const
{
    return m_part->view()->selectedItemsCount() > 0;
}

KParts::FileInfoExtension::QueryModes DolphinPartFileInfoExtension::supportedQueryModes() const
{
    return (KParts::FileInfoExtension::AllItems | KParts::FileInfoExtension::SelectedItems);
}

KFileItemList DolphinPartFileInfoExtension::queryFor(KParts::FileInfoExtension::QueryMode mode) const
{
    KFileItemList list;

    if (mode == KParts::FileInfoExtension::None)
        return list;

    if (!(supportedQueryModes() & mode))
        return list;

    switch (mode) {
    case KParts::FileInfoExtension::SelectedItems:
        if (hasSelection())
            return m_part->view()->selectedItems();
        break;
    case KParts::FileInfoExtension::AllItems:
        return m_part->view()->items();
    default:
        break;
    }

    return list;
}

DolphinPartListingFilterExtension::DolphinPartListingFilterExtension(DolphinPart *part)
    : KParts::ListingFilterExtension(part)
    , m_part(part)
{
}

KParts::ListingFilterExtension::FilterModes DolphinPartListingFilterExtension::supportedFilterModes() const
{
    return (KParts::ListingFilterExtension::MimeType | KParts::ListingFilterExtension::SubString | KParts::ListingFilterExtension::WildCard);
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

void DolphinPartListingFilterExtension::setFilter(KParts::ListingFilterExtension::FilterMode mode, const QVariant &filter)
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

DolphinPartListingNotificationExtension::DolphinPartListingNotificationExtension(DolphinPart *part)
    : KParts::ListingNotificationExtension(part)
{
}

KParts::ListingNotificationExtension::NotificationEventTypes DolphinPartListingNotificationExtension::supportedNotificationEventTypes() const
{
    return (KParts::ListingNotificationExtension::ItemsAdded | KParts::ListingNotificationExtension::ItemsDeleted);
}

void DolphinPartListingNotificationExtension::slotNewItems(const KFileItemList &items)
{
    Q_EMIT listingEvent(KParts::ListingNotificationExtension::ItemsAdded, items);
}

void DolphinPartListingNotificationExtension::slotItemsDeleted(const KFileItemList &items)
{
    Q_EMIT listingEvent(KParts::ListingNotificationExtension::ItemsDeleted, items);
}

#include "moc_dolphinpart_ext.cpp"
