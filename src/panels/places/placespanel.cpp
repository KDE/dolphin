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

#include <KBookmark>
#include <KBookmarkGroup>
#include <KBookmarkManager>
#include <KComponentData>
#include <KDebug>
#include <KIcon>
#include <kitemviews/kitemlistcontainer.h>
#include <kitemviews/kitemlistcontroller.h>
#include <kitemviews/kstandarditem.h>
#include <kitemviews/kstandarditemlistview.h>
#include <kitemviews/kstandarditemmodel.h>
#include <KStandardDirs>
#include <views/draganddrophelper.h>
#include <QVBoxLayout>
#include <QShowEvent>

PlacesPanel::PlacesPanel(QWidget* parent) :
    Panel(parent),
    m_controller(0),
    m_model(0),
    m_availableDevices(),
    m_bookmarkManager(0)
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
        const QString file = KStandardDirs::locateLocal("data", "kfileplaces/bookmarks.xml");
        m_bookmarkManager = KBookmarkManager::managerForFile(file, "kfileplaces");
        m_model = new KStandardItemModel(this);
        loadBookmarks();

        KStandardItemListView* view = new KStandardItemListView();

        m_controller = new KItemListController(m_model, view, this);
        m_controller->setSelectionBehavior(KItemListController::SingleSelection);
        connect(m_controller, SIGNAL(itemActivated(int)), this, SLOT(slotItemActivated(int)));
        connect(m_controller, SIGNAL(itemMiddleClicked(int)), this, SLOT(slotItemMiddleClicked(int)));

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
    const KStandardItem* item = m_model->item(index);
    if (item) {
        const KUrl url = item->dataValue("url").value<KUrl>();
        emit placeActivated(url);
    }
}

void PlacesPanel::slotItemMiddleClicked(int index)
{
    const KStandardItem* item = m_model->item(index);
    if (item) {
        const KUrl url = item->dataValue("url").value<KUrl>();
        emit placeMiddleClicked(url);
    }
}

void PlacesPanel::slotUrlsDropped(const KUrl& dest, QDropEvent* event, QWidget* parent)
{
    Q_UNUSED(parent);
    DragAndDropHelper::dropUrls(KFileItem(), dest, event);
}

void PlacesPanel::loadBookmarks()
{
    KBookmarkGroup root = m_bookmarkManager->root();
    KBookmark bookmark = root.first();
    QSet<QString> devices = m_availableDevices;

    while (!bookmark.isNull()) {
        const QString udi = bookmark.metaDataItem("UDI");
        const QString appName = bookmark.metaDataItem("OnlyInApp");
        const bool deviceAvailable = devices.remove(udi);

        const bool allowedHere = appName.isEmpty() || (appName == KGlobal::mainComponent().componentName());

        if ((udi.isEmpty() && allowedHere) || deviceAvailable) {
            KStandardItem* item = new KStandardItem();
            item->setIcon(KIcon(bookmark.icon()));
            item->setText(bookmark.text());
            item->setDataValue("address", bookmark.address());
            item->setDataValue("url", bookmark.url());
            if (deviceAvailable) {
                item->setDataValue("udi", udi);
            }
            m_model->appendItem(item);
        }

        bookmark = root.next(bookmark);
    }
}

#include "placespanel.moc"
