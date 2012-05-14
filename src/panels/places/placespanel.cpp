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

#ifdef HAVE_NEPOMUK
    #include <Nepomuk/Query/ComparisonTerm>
    #include <Nepomuk/Query/LiteralTerm>
    #include <Nepomuk/Query/Query>
    #include <Nepomuk/Query/ResourceTypeTerm>
    #include <Nepomuk/Vocabulary/NFO>
    #include <Nepomuk/Vocabulary/NIE>
#endif

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
        emit placeActivated(convertedUrl(url));
    }
}

void PlacesPanel::slotItemMiddleClicked(int index)
{
    const KUrl url = m_model->data(index).value("url").value<KUrl>();
    if (!url.isEmpty()) {
        emit placeMiddleClicked(convertedUrl(url));
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
            KConfig trashConfig("trashrc", KConfig::SimpleConfig);
            emptyTrashAction->setEnabled(!trashConfig.group("Status").readEntry("Empty", true));
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
        KStandardItem* item = createStandardItemFromDialog(dialog);

        // Insert the item as last item of the corresponding group.
        int i = 0;
        while (i < m_model->count() && m_model->item(i)->group() != item->group()) {
            ++i;
        }

        bool inserted = false;
        while (!inserted && i < m_model->count()) {
            if (m_model->item(i)->group() != item->group()) {
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
        KStandardItem* oldItem = m_model->item(index);
        if (oldItem) {
            KStandardItem* item = createStandardItemFromDialog(dialog);
            // Although the user might have changed the URL of the item in a way
            // that another group should be assigned, we still apply the old
            // group to keep the same position for the item.
            item->setGroup(oldItem->group());
            m_model->changeItem(index, item);
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

KStandardItem* PlacesPanel::createStandardItemFromDialog(PlacesItemEditDialog* dialog) const
{
    Q_ASSERT(dialog);

    const KUrl newUrl = dialog->url();
    KStandardItem* item = new KStandardItem();
    item->setIcon(dialog->icon());
    item->setText(dialog->text());
    item->setDataValue("url", newUrl);
    item->setGroup(m_model->groupName(newUrl));

    return item;
}

KUrl PlacesPanel::convertedUrl(const KUrl& url)
{
    KUrl newUrl = url;
    if (url.protocol() == QLatin1String("timeline")) {
        newUrl = createTimelineUrl(url);
    } else if (url.protocol() == QLatin1String("search")) {
        newUrl = createSearchUrl(url);
    }

    return newUrl;
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
#else
    Q_UNUSED(url);
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
