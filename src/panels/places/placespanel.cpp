/***************************************************************************
 *   Copyright (C) 2008-2012 by Peter Penz <peter.penz19@gmail.com>        *
 *                                                                         *
 *   Based on KFilePlacesView from kdelibs:                                *
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

#include <KConfigGroup>
#include <KIcon>
#include <KLocale>
#include <kitemviews/kitemlistcontainer.h>
#include <kitemviews/kitemlistcontroller.h>
#include <kitemviews/kstandarditemlistview.h>
#include <KMenu>
#include "placesitemlistgroupheader.h"
#include "placesitemmodel.h"
#include <views/draganddrophelper.h>
#include <QVBoxLayout>
#include <QShowEvent>

PlacesPanel::PlacesPanel(QWidget* parent) :
    Panel(parent),
    m_controller(0),
    m_model(0)
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
        m_model = new PlacesItemModel(this);
        m_model->setGroupedSorting(true);
        m_model->setSortRole("group");

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
    const KUrl url = m_model->data(index).value("url").value<KUrl>();
    if (!url.isEmpty()) {
        emit placeActivated(url);
    }
}

void PlacesPanel::slotItemMiddleClicked(int index)
{
    const KUrl url = m_model->data(index).value("url").value<KUrl>();
    if (!url.isEmpty()) {
        emit placeMiddleClicked(url);
    }
}

void PlacesPanel::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    const QHash<QByteArray, QVariant> data = m_model->data(index);
    const QString label = data.value("text").toString();

    KMenu menu(this);

    QAction* emptyTrash = 0;
    QAction* addEntry = 0;
    QAction* mainSeparator = 0;
    QAction* editEntry = 0;
    QAction* hideEntry = 0;

    const bool isDevice = !data.value("udi").toString().isEmpty();
    if (isDevice) {
    } else {
        if (data.value("url").value<KUrl>() == KUrl("trash:/")) {
            emptyTrash = menu.addAction(KIcon("trash-empty"), i18nc("@action:inmenu", "Empty Trash"));
            KConfig trashConfig("trashrc", KConfig::SimpleConfig);
            emptyTrash->setEnabled(!trashConfig.group("Status").readEntry("Empty", true));
            menu.addSeparator();
        }
        addEntry = menu.addAction(KIcon("document-new"), i18nc("@item:inmenu", "Add Entry..."));
        mainSeparator = menu.addSeparator();
        editEntry = menu.addAction(KIcon("document-properties"), i18n("&Edit Entry '%1'...", label));
    }

    if (!addEntry) {
        addEntry = menu.addAction(KIcon("document-new"), i18nc("@item:inmenu", "Add Entry..."));
    }

    menu.addSeparator();
    foreach (QAction* action, customContextMenuActions()) {
        menu.addAction(action);
    }

    QAction* action = menu.exec(pos.toPoint());
    hideEntry = menu.addAction(i18n("&Hide Entry '%1'", label));
    hideEntry->setCheckable(true);
    //hideEntry->setChecked(data.value("hidden").toBool());
    Q_UNUSED(action);
}

void PlacesPanel::slotViewContextMenuRequested(const QPointF& pos)
{
    KMenu menu(this);

    QAction* addEntry = menu.addAction(KIcon("document-new"), i18nc("@item:inmenu", "Add Entry..."));
    menu.addSeparator();
    foreach (QAction* action, customContextMenuActions()) {
        menu.addAction(action);
    }

    QAction* action = menu.exec(pos.toPoint());
    Q_UNUSED(action);
    Q_UNUSED(addEntry);
}

void PlacesPanel::slotUrlsDropped(const KUrl& dest, QDropEvent* event, QWidget* parent)
{
    Q_UNUSED(parent);
    DragAndDropHelper::dropUrls(KFileItem(), dest, event);
}

#include "placespanel.moc"
