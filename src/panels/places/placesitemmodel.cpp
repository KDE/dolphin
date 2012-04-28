/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on KFilePlacesModel from kdelibs:                               *
 *   Copyright (C) 2007 Kevin Ottens <ervin@kde.org>                       *
 *   Copyright (C) 2007 David Faure <faure@kde.org>                        *
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

#include "placesitemmodel.h"

#ifdef HAVE_NEPOMUK
    #include <Nepomuk/ResourceManager>
    #include <Nepomuk/Query/ComparisonTerm>
    #include <Nepomuk/Query/LiteralTerm>
    #include <Nepomuk/Query/Query>
    #include <Nepomuk/Query/ResourceTypeTerm>
    #include <Nepomuk/Vocabulary/NFO>
    #include <Nepomuk/Vocabulary/NIE>
#endif

#include <KBookmark>
#include <KBookmarkGroup>
#include <KBookmarkManager>
#include <KComponentData>
#include <KIcon>
#include <kitemviews/kstandarditem.h>
#include <KLocale>
#include <KStandardDirs>
#include <KUser>
#include <QDate>

PlacesItemModel::PlacesItemModel(QObject* parent) :
    KStandardItemModel(parent),
    m_nepomukRunning(false),
    m_availableDevices(),
    m_bookmarkManager(0),
    m_systemBookmarks(),
    m_systemBookmarksIndexes()
{
#ifdef HAVE_NEPOMUK
    m_nepomukRunning = (Nepomuk::ResourceManager::instance()->initialized());
#endif
    const QString file = KStandardDirs::locateLocal("data", "kfileplaces/bookmarks.xml");
    m_bookmarkManager = KBookmarkManager::managerForFile(file, "kfilePlaces");

    createSystemBookmarks();
    loadBookmarks();
}

PlacesItemModel::~PlacesItemModel()
{
}

int PlacesItemModel::hiddenCount() const
{
    return 0;
}

bool PlacesItemModel::isSystemItem(int index) const
{
    if (index >= 0 && index < count()) {
        const KUrl url = data(index).value("url").value<KUrl>();
        return m_systemBookmarksIndexes.contains(url);
    }
    return false;
}

int PlacesItemModel::closestItem(const KUrl& url) const
{
    int foundIndex = -1;
    int maxLength = 0;

    for (int i = 0; i < count(); ++i) {
        const KUrl itemUrl = data(i).value("url").value<KUrl>();
        if (itemUrl.isParentOf(url)) {
            const int length = itemUrl.prettyUrl().length();
            if (length > maxLength) {
                foundIndex = i;
                maxLength = length;
            }
        }
    }

    return foundIndex;
}

QString PlacesItemModel::placesGroupName() const
{
    return i18nc("@item", "Places");
}

QString PlacesItemModel::recentlyAccessedGroupName() const
{
    return i18nc("@item", "Recently Accessed");
}

QString PlacesItemModel::searchForGroupName() const
{
    return i18nc("@item", "Search For");
}

QAction* PlacesItemModel::ejectAction(int index) const
{
    Q_UNUSED(index);
    return 0;
}

QAction* PlacesItemModel::tearDownAction(int index) const
{
    Q_UNUSED(index);
    return 0;
}

void PlacesItemModel::loadBookmarks()
{
    KBookmarkGroup root = m_bookmarkManager->root();
    KBookmark bookmark = root.first();
    QSet<QString> devices = m_availableDevices;

    QSet<KUrl> missingSystemBookmarks;
    foreach (const SystemBookmarkData& data, m_systemBookmarks) {
        missingSystemBookmarks.insert(data.url);
    }

    while (!bookmark.isNull()) {
        const QString udi = bookmark.metaDataItem("UDI");
        const KUrl url = bookmark.url();
        const QString appName = bookmark.metaDataItem("OnlyInApp");
        const bool deviceAvailable = devices.remove(udi);

        const bool allowedHere = (appName.isEmpty() || appName == KGlobal::mainComponent().componentName())
                                 && (m_nepomukRunning || url.protocol() != QLatin1String("timeline"));

        if ((udi.isEmpty() && allowedHere) || deviceAvailable) {
            KStandardItem* item = new KStandardItem();
            item->setIcon(KIcon(bookmark.icon()));
            item->setDataValue("address", bookmark.address());
            item->setDataValue("url", url);

            if (missingSystemBookmarks.contains(url)) {
                missingSystemBookmarks.remove(url);
                // Apply the translated text to the system bookmarks, otherwise an outdated
                // translation might be shown.
                const int index = m_systemBookmarksIndexes.value(url);
                item->setText(m_systemBookmarks[index].text);

                // The system bookmarks don't contain "real" queries stored as URLs, so
                // they must be translated first.
                item->setDataValue("url", translatedSystemBookmarkUrl(url));
            } else {
                item->setText(bookmark.text());
            }

            if (deviceAvailable) {
                item->setDataValue("udi", udi);
                item->setGroup(i18nc("@item", "Devices"));
            } else {
                item->setGroup(i18nc("@item", "Places"));
            }

            appendItem(item);
        }

        bookmark = root.next(bookmark);
    }

    if (!missingSystemBookmarks.isEmpty()) {
        foreach (const SystemBookmarkData& data, m_systemBookmarks) {
            if (missingSystemBookmarks.contains(data.url)) {
                KStandardItem* item = new KStandardItem();
                item->setIcon(KIcon(data.icon));
                item->setText(data.text);
                item->setDataValue("url", translatedSystemBookmarkUrl(data.url));
                item->setGroup(data.group);
                appendItem(item);
            }
        }
    }
}

void PlacesItemModel::createSystemBookmarks()
{
    Q_ASSERT(m_systemBookmarks.isEmpty());
    Q_ASSERT(m_systemBookmarksIndexes.isEmpty());

    const QString placesGroup = placesGroupName();
    const QString recentlyAccessedGroup = recentlyAccessedGroupName();
    const QString searchForGroup = searchForGroupName();
    const QString timeLineIcon = "package_utility_time"; // TODO: Ask the Oxygen team to create
                                                         // a custom icon for the timeline-protocol

    m_systemBookmarks.append(SystemBookmarkData(KUrl(KUser().homeDir()),
                                                "user-home",
                                                i18nc("@item", "Home"),
                                                placesGroup));
    m_systemBookmarks.append(SystemBookmarkData(KUrl("remote:/"),
                                                "network-workgroup",
                                                i18nc("@item", "Network"),
                                                placesGroup));
    m_systemBookmarks.append(SystemBookmarkData(KUrl("/"),
                                                "folder-red",
                                                i18nc("@item", "Root"),
                                                placesGroup));
    m_systemBookmarks.append(SystemBookmarkData(KUrl("trash:/"),
                                                "user-trash",
                                                i18nc("@item", "Trash"),
                                                placesGroup));

    if (m_nepomukRunning) {
        m_systemBookmarks.append(SystemBookmarkData(KUrl("timeline:/today"),
                                                    timeLineIcon,
                                                    i18nc("@item Recently Accessed", "Today"),
                                                    recentlyAccessedGroup));
        m_systemBookmarks.append(SystemBookmarkData(KUrl("timeline:/yesterday"),
                                                    timeLineIcon,
                                                    i18nc("@item Recently Accessed", "Yesterday"),
                                                    recentlyAccessedGroup));
        m_systemBookmarks.append(SystemBookmarkData(KUrl("timeline:/thismonth"),
                                                    timeLineIcon,
                                                    i18nc("@item Recently Accessed", "This Month"),
                                                    recentlyAccessedGroup));
        m_systemBookmarks.append(SystemBookmarkData(KUrl("timeline:/lastmonth"),
                                                    timeLineIcon,
                                                    i18nc("@item Recently Accessed", "Last Month"),
                                                    recentlyAccessedGroup));
        m_systemBookmarks.append(SystemBookmarkData(KUrl("search:/documents"),
                                                    "folder-txt",
                                                    i18nc("@item Commonly Accessed", "Documents"),
                                                    searchForGroup));
        m_systemBookmarks.append(SystemBookmarkData(KUrl("search:/images"),
                                                    "folder-image",
                                                    i18nc("@item Commonly Accessed", "Images"),
                                                    searchForGroup));
        m_systemBookmarks.append(SystemBookmarkData(KUrl("search:/audio"),
                                                    "folder-sound",
                                                    i18nc("@item Commonly Accessed", "Audio"),
                                                    searchForGroup));
        m_systemBookmarks.append(SystemBookmarkData(KUrl("search:/videos"),
                                                    "folder-video",
                                                    i18nc("@item Commonly Accessed", "Videos"),
                                                    searchForGroup));
    }

    for (int i = 0; i < m_systemBookmarks.count(); ++i) {
        const KUrl url = translatedSystemBookmarkUrl(m_systemBookmarks[i].url);
        m_systemBookmarksIndexes.insert(url, i);
    }
}


KUrl PlacesItemModel::translatedSystemBookmarkUrl(const KUrl& url) const
{
    KUrl translatedUrl = url;
    if (url.protocol() == QLatin1String("timeline")) {
        translatedUrl = createTimelineUrl(url);
    } else if (url.protocol() == QLatin1String("search")) {
        translatedUrl = createSearchUrl(url);
    }

    return translatedUrl;
}

KUrl PlacesItemModel::createTimelineUrl(const KUrl& url)
{
    // TODO: Clarify with the Nepomuk-team whether it makes sense
    // provide default-timeline-URLs like 'yesterday', 'this month'
    // and 'last month'.
    KUrl timelineUrl;

    const QString path = url.pathOrUrl();
    if (path.endsWith("yesterday")) {
        const QDate date = QDate::currentDate().addDays(-1);
        const int year = date.year();
        const int month = date.month();
        const int day = date.day();
        timelineUrl = "timeline:/" + timelineDateString(year, month) +
              '/' + timelineDateString(year, month, day);
    } else if (path.endsWith("thismonth")) {
        const QDate date = QDate::currentDate();
        timelineUrl = "timeline:/" + timelineDateString(date.year(), date.month());
    } else if (path.endsWith("lastmonth")) {
        const QDate date = QDate::currentDate().addMonths(-1);
        timelineUrl = "timeline:/" + timelineDateString(date.year(), date.month());
    } else {
        Q_ASSERT(path.endsWith("today"));
        timelineUrl= url;
    }

    return timelineUrl;
}

QString PlacesItemModel::timelineDateString(int year, int month, int day)
{
    QString date = QString::number(year) + '-';
    if (month < 10) {
        date += '0';
    }
    date += QString::number(month);

    if (day >= 1) {
        date += '-';
        if (day < 10) {
            date += '0';
        }
        date += QString::number(day);
    }

    return date;
}

KUrl PlacesItemModel::createSearchUrl(const KUrl& url)
{
    KUrl searchUrl;

#ifdef HAVE_NEPOMUK
    const QString path = url.pathOrUrl();
    if (path.endsWith("documents")) {
        searchUrl = searchUrlForTerm(Nepomuk::Query::ResourceTypeTerm(Nepomuk::Vocabulary::NFO::Document()));
    } else if (path.endsWith("images")) {
        searchUrl = searchUrlForTerm(Nepomuk::Query::ResourceTypeTerm(Nepomuk::Vocabulary::NFO::Image()));
    } else if (path.endsWith("audio")) {
        searchUrl = searchUrlForTerm(Nepomuk::Query::ComparisonTerm(Nepomuk::Vocabulary::NIE::mimeType(),
                                                                    Nepomuk::Query::LiteralTerm("audio")));
    } else if (path.endsWith("videos")) {
        searchUrl = searchUrlForTerm(Nepomuk::Query::ComparisonTerm(Nepomuk::Vocabulary::NIE::mimeType(),
                                                                    Nepomuk::Query::LiteralTerm("video")));
    } else {
        Q_ASSERT(false);
    }
#else
    Q_UNUSED(url);
#endif

    return searchUrl;
}

#ifdef HAVE_NEPOMUK
KUrl PlacesItemModel::searchUrlForTerm(const Nepomuk::Query::Term& term)
{
    const Nepomuk::Query::Query query(term);
    return query.toSearchUrl();
}
#endif

#include "placesitemmodel.moc"
