/***************************************************************************
 *   Copyright (C) 2008-2012 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "placespanel.h"

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
#include <KDebug>
#include <KIcon>
#include <KLocale>
#include <kitemviews/kitemlistcontainer.h>
#include <kitemviews/kitemlistcontroller.h>
#include <kitemviews/kstandarditem.h>
#include <kitemviews/kstandarditemlistview.h>
#include <kitemviews/kstandarditemmodel.h>
#include <KStandardDirs>
#include <KUser>
#include "placesitemlistgroupheader.h"
#include <views/draganddrophelper.h>
#include <QDir>
#include <QVBoxLayout>
#include <QShowEvent>

PlacesPanel::PlacesPanel(QWidget* parent) :
    Panel(parent),
    m_nepomukRunning(false),
    m_controller(0),
    m_model(0),
    m_availableDevices(),
    m_bookmarkManager(0),
    m_defaultBookmarks(),
    m_defaultBookmarksIndexes()
{
}

PlacesPanel::~PlacesPanel()
{
}

bool PlacesPanel::urlChanged()
{
    return true;
}

void PlacesPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (!m_controller) {
        // Postpone the creating of the controller to the first show event.
        // This assures that no performance and memory overhead is given when the folders panel is not
        // used at all and stays invisible.
#ifdef HAVE_NEPOMUK
        m_nepomukRunning = (Nepomuk::ResourceManager::instance()->initialized());
#endif
        createDefaultBookmarks();

        const QString file = KStandardDirs::locateLocal("data", "kfileplaces/bookmarks.xml");
        m_bookmarkManager = KBookmarkManager::managerForFile(file, "kfilePlaces");
        m_model = new KStandardItemModel(this);
        m_model->setGroupedSorting(true);
        m_model->setSortRole("group");
        loadBookmarks();

        KStandardItemListView* view = new KStandardItemListView();
        view->setGroupHeaderCreator(new KItemListGroupHeaderCreator<PlacesItemListGroupHeader>());

        m_controller = new KItemListController(m_model, view, this);
        m_controller->setSelectionBehavior(KItemListController::SingleSelection);
        connect(m_controller, SIGNAL(itemActivated(int)), this, SLOT(slotItemActivated(int)));
        connect(m_controller, SIGNAL(itemMiddleClicked(int)), this, SLOT(slotItemMiddleClicked(int)));
        connect(m_controller, SIGNAL(itemContextMenuRequested(int,QPointF)), this, SLOT(slotItemContextMenuRequested(int,QPointF)));
        connect(m_controller, SIGNAL(viewContextMenuRequested(QPointF)), this, SLOT(slotViewContextMenuRequested(QPointF)));

        KItemListContainer* container = new KItemListContainer(m_controller, this);
        container->setEnabledFrame(false);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(container);
    }

    Panel::showEvent(event);
}

void PlacesPanel::slotItemActivated(int index)
{
    const KUrl url = urlForIndex(index);
    if (!url.isEmpty()) {
        emit placeActivated(url);
    }
}

void PlacesPanel::slotItemMiddleClicked(int index)
{
    const KUrl url = urlForIndex(index);
    if (!url.isEmpty()) {
        emit placeMiddleClicked(url);
    }
}

void PlacesPanel::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    Q_UNUSED(index);
    Q_UNUSED(pos);
}

void PlacesPanel::slotViewContextMenuRequested(const QPointF& pos)
{
    Q_UNUSED(pos);
}

void PlacesPanel::slotUrlsDropped(const KUrl& dest, QDropEvent* event, QWidget* parent)
{
    Q_UNUSED(parent);
    DragAndDropHelper::dropUrls(KFileItem(), dest, event);
}

void PlacesPanel::createDefaultBookmarks()
{
    Q_ASSERT(m_defaultBookmarks.isEmpty());
    Q_ASSERT(m_defaultBookmarksIndexes.isEmpty());

    const QString placesGroup = i18nc("@item", "Places");
    const QString recentlyAccessedGroup = i18nc("@item", "Recently Accessed");
    const QString searchForGroup = i18nc("@item", "Search For");
    const QString timeLineIcon = "package_utility_time"; // TODO: Ask the Oxygen team to create
                                                         // a custom icon for the timeline-protocol

    m_defaultBookmarks.append(DefaultBookmarkData(KUrl(KUser().homeDir()),
                                                  "user-home",
                                                  i18nc("@item", "Home"),
                                                  placesGroup));
    m_defaultBookmarks.append(DefaultBookmarkData(KUrl("remote:/"),
                                                  "network-workgroup",
                                                  i18nc("@item", "Network"),
                                                  placesGroup));
    m_defaultBookmarks.append(DefaultBookmarkData(KUrl("/"),
                                                  "folder-red",
                                                  i18nc("@item", "Root"),
                                                  placesGroup));
    m_defaultBookmarks.append(DefaultBookmarkData(KUrl("trash:/"),
                                                  "user-trash",
                                                  i18nc("@item", "Trash"),
                                                  placesGroup));

    if (m_nepomukRunning) {
        m_defaultBookmarks.append(DefaultBookmarkData(KUrl("timeline:/today"),
                                                      timeLineIcon,
                                                      i18nc("@item Recently Accessed", "Today"),
                                                      recentlyAccessedGroup));
        m_defaultBookmarks.append(DefaultBookmarkData(KUrl("timeline:/yesterday"),
                                                      timeLineIcon,
                                                      i18nc("@item Recently Accessed", "Yesterday"),
                                                      recentlyAccessedGroup));
        m_defaultBookmarks.append(DefaultBookmarkData(KUrl("timeline:/thismonth"),
                                                      timeLineIcon,
                                                      i18nc("@item Recently Accessed", "This Month"),
                                                      recentlyAccessedGroup));
        m_defaultBookmarks.append(DefaultBookmarkData(KUrl("timeline:/lastmonth"),
                                                      timeLineIcon,
                                                      i18nc("@item Recently Accessed", "Last Month"),
                                                      recentlyAccessedGroup));
        m_defaultBookmarks.append(DefaultBookmarkData(KUrl("search:/documents"),
                                                      "folder-txt",
                                                      i18nc("@item Commonly Accessed", "Documents"),
                                                      searchForGroup));
        m_defaultBookmarks.append(DefaultBookmarkData(KUrl("search:/images"),
                                                      "folder-image",
                                                      i18nc("@item Commonly Accessed", "Images"),
                                                      searchForGroup));
        m_defaultBookmarks.append(DefaultBookmarkData(KUrl("search:/audio"),
                                                      "folder-sound",
                                                      i18nc("@item Commonly Accessed", "Audio"),
                                                      searchForGroup));
        m_defaultBookmarks.append(DefaultBookmarkData(KUrl("search:/videos"),
                                                      "folder-video",
                                                      i18nc("@item Commonly Accessed", "Videos"),
                                                      searchForGroup));
    }

    for (int i = 0; i < m_defaultBookmarks.count(); ++i) {
        m_defaultBookmarksIndexes.insert(m_defaultBookmarks[i].url, i);
    }
}

void PlacesPanel::loadBookmarks()
{
    KBookmarkGroup root = m_bookmarkManager->root();
    KBookmark bookmark = root.first();
    QSet<QString> devices = m_availableDevices;

    QSet<KUrl> missingDefaultBookmarks;
    foreach (const DefaultBookmarkData& data, m_defaultBookmarks) {
        missingDefaultBookmarks.insert(data.url);
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

            if (missingDefaultBookmarks.contains(url)) {
                missingDefaultBookmarks.remove(url);
                // Always apply the translated text to the default bookmarks, otherwise an outdated
                // translation might be shown.
                const int index = m_defaultBookmarksIndexes.value(url);
                item->setText(m_defaultBookmarks[index].text);
            } else {
                item->setText(bookmark.text());
            }

            if (deviceAvailable) {
                item->setDataValue("udi", udi);
                item->setGroup(i18nc("@item", "Devices"));
            } else {
                item->setGroup(i18nc("@item", "Places"));
            }

            m_model->appendItem(item);
        }

        bookmark = root.next(bookmark);
    }

    if (!missingDefaultBookmarks.isEmpty()) {
        foreach (const DefaultBookmarkData& data, m_defaultBookmarks) {
            if (missingDefaultBookmarks.contains(data.url)) {
                KStandardItem* item = new KStandardItem();
                item->setIcon(KIcon(data.icon));
                item->setText(data.text);
                item->setDataValue("url", data.url);
                item->setGroup(data.group);
                m_model->appendItem(item);
            }
        }
    }
}

KUrl PlacesPanel::urlForIndex(int index) const
{
    const KStandardItem* item = m_model->item(index);
    if (!item) {
        return KUrl();
    }

    KUrl url = item->dataValue("url").value<KUrl>();
    if (url.protocol() == QLatin1String("timeline")) {
        url = createTimelineUrl(url);
    } else if (url.protocol() == QLatin1String("search")) {
        url = createSearchUrl(url);
    }

    return url;
}

KUrl PlacesPanel::createTimelineUrl(const KUrl& url)
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

QString PlacesPanel::timelineDateString(int year, int month, int day)
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

KUrl PlacesPanel::createSearchUrl(const KUrl& url)
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
#endif

    return searchUrl;
}

#ifdef HAVE_NEPOMUK
KUrl PlacesPanel::searchUrlForTerm(const Nepomuk::Query::Term& term)
{
    const Nepomuk::Query::Query query(term);
    return query.toSearchUrl();
}
#endif

#include "placespanel.moc"
