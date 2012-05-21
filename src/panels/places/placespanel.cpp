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

#include <KDebug>
#include <KDirNotify>
#include <KIcon>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KLocale>
#include <kitemviews/kitemlistcontainer.h>
#include <kitemviews/kitemlistcontroller.h>
#include <kitemviews/kitemlistselectionmanager.h>
#include <kitemviews/kstandarditem.h>
#include <kitemviews/kstandarditemlistview.h>
#include <KMenu>
#include <KMessageBox>
#include <KNotification>
#include "placesitem.h"
#include "placesitemeditdialog.h"
#include "placesitemlistgroupheader.h"
#include "placesitemlistwidget.h"
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
        connect(m_model, SIGNAL(errorMessage(QString)),
                this, SIGNAL(errorMessage(QString)));

        KStandardItemListView* view = new KStandardItemListView();
        view->setWidgetCreator(new KItemListWidgetCreator<PlacesItemListWidget>());
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

        selectClosestItem();
    }

    Panel::showEvent(event);
}

void PlacesPanel::slotItemActivated(int index)
{
    const KUrl url = m_model->data(index).value("url").value<KUrl>();
    if (!url.isEmpty()) {
        emit placeActivated(PlacesItemModel::convertedUrl(url));
    }
}

void PlacesPanel::slotItemMiddleClicked(int index)
{
    const KUrl url = m_model->data(index).value("url").value<KUrl>();
    if (!url.isEmpty()) {
        emit placeMiddleClicked(PlacesItemModel::convertedUrl(url));
    }
}

void PlacesPanel::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    PlacesItem* item = m_model->placesItem(index);
    if (!item) {
        return;
    }

    KMenu menu(this);

    QAction* emptyTrashAction = 0;
    QAction* addAction = 0;
    QAction* mainSeparator = 0;
    QAction* editAction = 0;
    QAction* teardownAction = 0;
    QAction* ejectAction = 0;

    const QString label = item->text();

    const bool isDevice = !item->udi().isEmpty();
    if (isDevice) {
        ejectAction = m_model->ejectAction(index);
        if (ejectAction) {
            ejectAction->setParent(&menu);
            menu.addAction(ejectAction);
        }

        teardownAction = m_model->teardownAction(index);
        if (teardownAction) {
            teardownAction->setParent(&menu);
            menu.addAction(teardownAction);
        }

        if (teardownAction || ejectAction) {
            mainSeparator = menu.addSeparator();
        }
    } else {
        if (item->url() == KUrl("trash:/")) {
            emptyTrashAction = menu.addAction(KIcon("trash-empty"), i18nc("@action:inmenu", "Empty Trash"));
            emptyTrashAction->setEnabled(item->icon() == "user-trash-full");
            menu.addSeparator();
        }
        addAction = menu.addAction(KIcon("document-new"), i18nc("@item:inmenu", "Add Entry..."));
        mainSeparator = menu.addSeparator();
        editAction = menu.addAction(KIcon("document-properties"), i18nc("@item:inmenu", "Edit '%1'...", label));
    }

    if (!addAction) {
        addAction = menu.addAction(KIcon("document-new"), i18nc("@item:inmenu", "Add Entry..."));
    }

    QAction* openInNewTabAction = menu.addAction(i18nc("@item:inmenu", "Open '%1' in New Tab", label));
    openInNewTabAction->setIcon(KIcon("tab-new"));

    QAction* removeAction = 0;
    if (!isDevice && !item->isSystemItem()) {
        removeAction = menu.addAction(KIcon("edit-delete"), i18nc("@item:inmenu", "Remove '%1'", label));
    }

    QAction* hideAction = menu.addAction(i18nc("@item:inmenu", "Hide '%1'", label));
    hideAction->setCheckable(true);
    hideAction->setChecked(item->isHidden());

    QAction* showAllAction = 0;
    if (m_model->hiddenCount() > 0) {
        if (!mainSeparator) {
            mainSeparator = menu.addSeparator();
        }
        showAllAction = menu.addAction(i18nc("@item:inmenu", "Show All Entries"));
        showAllAction->setCheckable(true);
        showAllAction->setChecked(m_model->hiddenItemsShown());
    }

    menu.addSeparator();
    foreach (QAction* action, customContextMenuActions()) {
        menu.addAction(action);
    }

    QAction* action = menu.exec(pos.toPoint());
    if (action) {
        if (action == emptyTrashAction) {
            emptyTrash();
        } else if (action == addAction) {
            addEntry();
        } else if (action == editAction) {
            editEntry(index);
        } else if (action == removeAction) {
            m_model->removeItem(index);
        } else if (action == hideAction) {
            item->setHidden(hideAction->isChecked());
        } else if (action == openInNewTabAction) {
            const KUrl url = m_model->item(index)->dataValue("url").value<KUrl>();
            emit placeMiddleClicked(url);
        } else if (action == showAllAction) {
            m_model->setHiddenItemsShown(showAllAction->isChecked());
        } else if (action == teardownAction) {
            m_model->requestTeardown(index);
        } else if (action == ejectAction) {
            m_model->requestEject(index);
        }
    }

    selectClosestItem();
}

void PlacesPanel::slotViewContextMenuRequested(const QPointF& pos)
{
    KMenu menu(this);

    QAction* addAction = menu.addAction(KIcon("document-new"), i18nc("@item:inmenu", "Add Entry..."));

    QAction* showAllAction = 0;
    if (m_model->hiddenCount() > 0) {
        showAllAction = menu.addAction(i18nc("@item:inmenu", "Show All Entries"));
        showAllAction->setCheckable(true);
        showAllAction->setChecked(m_model->hiddenItemsShown());
    }

    menu.addSeparator();
    foreach (QAction* action, customContextMenuActions()) {
        menu.addAction(action);
    }

    QAction* action = menu.exec(pos.toPoint());
    if (action) {
        if (action == addAction) {
            addEntry();
        } else if (action == showAllAction) {
            m_model->setHiddenItemsShown(showAllAction->isChecked());
        }
    }

    selectClosestItem();
}

void PlacesPanel::slotUrlsDropped(const KUrl& dest, QDropEvent* event, QWidget* parent)
{
    Q_UNUSED(parent);
    const QString error = DragAndDropHelper::dropUrls(KFileItem(), dest, event);
    if (!error.isEmpty()) {
        emit errorMessage(error);
    }

}

void PlacesPanel::slotTrashUpdated(KJob* job)
{
    if (job->error()) {
        emit errorMessage(job->errorString());
    }
    org::kde::KDirNotify::emitFilesAdded("trash:/");
}

void PlacesPanel::emptyTrash()
{
    const QString text = i18nc("@info", "Do you really want to empty the Trash? All items will be deleted.");
    const bool del = KMessageBox::warningContinueCancel(window(),
                                                        text,
                                                        QString(),
                                                        KGuiItem(i18nc("@action:button", "Empty Trash"),
                                                                 KIcon("user-trash"))
                                                       ) == KMessageBox::Continue;
    if (del) {
        QByteArray packedArgs;
        QDataStream stream(&packedArgs, QIODevice::WriteOnly);
        stream << int(1);
        KIO::Job *job = KIO::special(KUrl("trash:/"), packedArgs);
        KNotification::event("Trash: emptied", QString() , QPixmap() , 0, KNotification::DefaultEvent);
        job->ui()->setWindow(parentWidget());
        connect(job, SIGNAL(result(KJob*)), SLOT(slotTrashUpdated(KJob*)));
    }
}

void PlacesPanel::addEntry()
{
    const int index = m_controller->selectionManager()->currentItem();
    const KUrl url = m_model->data(index).value("url").value<KUrl>();

    QPointer<PlacesItemEditDialog> dialog = new PlacesItemEditDialog(this);
    dialog->setCaption(i18nc("@title:window", "Add Places Entry"));
    dialog->setAllowGlobal(true);
    dialog->setUrl(url);
    if (dialog->exec() == QDialog::Accepted) {
        PlacesItem* item = m_model->createPlacesItem(dialog->text(), dialog->url(), dialog->icon());

        // Insert the item as last item of the corresponding group.
        int i = 0;
        while (i < m_model->count() && m_model->placesItem(i)->group() != item->group()) {
            ++i;
        }

        bool inserted = false;
        while (!inserted && i < m_model->count()) {
            if (m_model->placesItem(i)->group() != item->group()) {
                m_model->insertItem(i, item);
                inserted = true;
            }
            ++i;
        }

        if (!inserted) {
            m_model->appendItem(item);
        }
    }

    delete dialog;
}

void PlacesPanel::editEntry(int index)
{
    QHash<QByteArray, QVariant> data = m_model->data(index);

    QPointer<PlacesItemEditDialog> dialog = new PlacesItemEditDialog(this);
    dialog->setCaption(i18nc("@title:window", "Edit Places Entry"));
    dialog->setIcon(data.value("iconName").toString());
    dialog->setText(data.value("text").toString());
    dialog->setUrl(data.value("url").value<KUrl>());
    dialog->setAllowGlobal(true);
    if (dialog->exec() == QDialog::Accepted) {
        PlacesItem* oldItem = m_model->placesItem(index);
        if (oldItem) {
            oldItem->setText(dialog->text());
            oldItem->setUrl(dialog->url());
            oldItem->setIcon(dialog->icon());
        }
    }

    delete dialog;
}

void PlacesPanel::selectClosestItem()
{
    const int index = m_model->closestItem(url());
    KItemListSelectionManager* selectionManager = m_controller->selectionManager();
    selectionManager->setCurrentItem(index);
    selectionManager->clearSelection();
    selectionManager->setSelected(index);
}

#include "placespanel.moc"
