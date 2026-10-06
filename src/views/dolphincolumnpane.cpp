/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphincolumnpane.h"

#include "dolphin_columnsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphinitemlistview.h"
#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistheader.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kitemliststyleoption.h"
#include "versioncontrol/versioncontrolobserver.h"
#include "views/dolphinview.h"

#include <QScrollBar>
#include <QStyle>
#include <QStyleOption>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

/// The narrowest a column can be dragged.
static constexpr int s_minimumWidth = 100;

DolphinColumnPane::DolphinColumnPane(KFileItemModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    m_view = new DolphinItemListView();
    m_view->setVisibleRoles({"text"});
    m_view->setViewMode(DolphinView::ColumnsView);
    m_view->setItemLayout(KFileItemListView::DetailsLayout);
    m_view->setHeaderVisible(false);
    m_view->setAlternateBackgrounds(false);
    m_view->setEnabledSelectionToggles(DolphinItemListView::False);
    m_view->setHighlightEntireRow(true);

    // The full-row highlight is drawn one padding wider than the row on each side.
    const int sidePadding = 2 * m_view->styleOption().padding;
    m_view->header()->setSidePadding(sidePadding, sidePadding);

    m_controller = new KItemListController(m_model, m_view, this);
    m_controller->setSelectionBehavior(KItemListController::MultiSelection);

    m_container = new KItemListContainer(m_controller, this);
    m_container->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_view->setAccessibleParentsObject(m_container);

    layout->addWidget(m_container);

    // No setView(): it needs a DolphinView and only matters on activation.
    m_versionControlObserver = new VersionControlObserver(this);
    m_versionControlObserver->setModel(m_model);
    connect(m_versionControlObserver, &VersionControlObserver::infoMessage, this, &DolphinColumnPane::infoMessage);
    connect(m_versionControlObserver, &VersionControlObserver::errorMessage, this, [this](const QString &msg) {
        Q_EMIT errorMessage(msg);
    });
    connect(m_versionControlObserver, &VersionControlObserver::operationCompletedMessage, this, &DolphinColumnPane::operationCompletedMessage);

    setMinimumWidth(s_minimumWidth);

    connect(m_controller, &KItemListController::itemActivated, this, &DolphinColumnPane::slotItemActivated);
    connect(m_controller->selectionManager(), &KItemListSelectionManager::currentChanged, this, &DolphinColumnPane::slotCurrentChanged);
    connect(m_controller->selectionManager(), &KItemListSelectionManager::selectionChanged, this, &DolphinColumnPane::slotSelectionChanged);
    connect(m_model, &KFileItemModel::directoryLoadingCompleted, this, &DolphinColumnPane::directoryLoadingCompleted);
}

DolphinColumnPane::~DolphinColumnPane() = default;

QUrl DolphinColumnPane::dirUrl() const
{
    return m_model->directory();
}

KFileItem DolphinColumnPane::currentFileItem() const
{
    return m_model->fileItem(m_controller->selectionManager()->currentItem());
}

void DolphinColumnPane::setActiveChildUrl(const QUrl &childUrl)
{
    if (childUrl.isEmpty()) {
        m_activeChildUrl.clear();
        clearActiveChild();
        return;
    }

    // Kept for reapplyActiveChildMark(), the item may not be listed yet.
    m_activeChildUrl = childUrl;

    const KFileItem item = m_model->fileItem(childUrl);
    if (!item.isNull()) {
        const int index = m_model->index(item);
        if (index >= 0) {
            m_controller->selectionManager()->clearSelection();
            m_controller->selectionManager()->setCurrentItem(index);
            m_controller->selectionManager()->setSelected(index, 1, KItemListSelectionManager::Select);
        }
    }
}

void DolphinColumnPane::setWidthPending(bool pending)
{
    m_widthPending = pending;
    // qSmartMinSize() follows minimumSizeHint(), so only the maximum width brings it to zero.
    setMinimumWidth(pending ? 0 : s_minimumWidth);
    setMaximumWidth(pending ? 0 : QWIDGETSIZE_MAX);
}

bool DolphinColumnPane::isWidthPending() const
{
    return m_widthPending;
}

void DolphinColumnPane::reapplyActiveChildMark()
{
    if (m_activeChildUrl.isEmpty()) {
        return;
    }
    const int index = m_model->index(m_model->fileItem(m_activeChildUrl));
    const KItemListSelectionManager *selectionManager = m_controller->selectionManager();
    if (index >= 0 && selectionManager->isSelected(index) && selectionManager->currentItem() == index) {
        return;
    }
    setActiveChildUrl(m_activeChildUrl);
}

void DolphinColumnPane::dropActiveChild()
{
    m_controller->selectionManager()->clearSelection();
    if (!m_activeChildUrl.isEmpty()) {
        m_activeChildUrl.clear();
        Q_EMIT activeChildRemoved();
    }
}

void DolphinColumnPane::clearActiveChild()
{
    m_controller->selectionManager()->clearSelection();
}

KFileItemModel *DolphinColumnPane::model() const
{
    return m_model;
}

KItemListController *DolphinColumnPane::controller() const
{
    return m_controller;
}

KItemListContainer *DolphinColumnPane::container() const
{
    return m_container;
}

DolphinItemListView *DolphinColumnPane::itemListView() const
{
    return m_view;
}

void DolphinColumnPane::setPreviewsShown(bool show)
{
    m_view->setPreviewsShown(show);
}

int DolphinColumnPane::calculateOptimalWidth() const
{
    const KItemListStyleOption &option = m_view->styleOption();
    const QFontMetrics &fm = option.fontMetrics;
    qreal maxWidth = 0;

    const bool hiddenFilesShown = m_model->showHiddenFiles();

    for (int i = 0; i < m_model->count(); ++i) {
        if (!hiddenFilesShown && m_model->fileItem(i).isHidden()) {
            continue;
        }
        const QString text = m_model->data(i).value("text").toString();
        qreal width = option.padding * 6; // matches KStandardItemListWidget::columnPadding()
        width += option.padding * 2 + option.iconSize;
        width += fm.horizontalAdvance(text);
        maxWidth = qMax(maxWidth, width);
    }

    // The frame and the scrollbar, even while it is hidden, so the width holds when it appears.
    QStyleOption styleOpt;
    styleOpt.initFrom(m_container);
    int scrollBarSpacing = 0;
    if (m_container->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &styleOpt, m_container)) {
        scrollBarSpacing = m_container->style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, &styleOpt, m_container);
    }
    const int chrome = m_container->frameWidth() * 2 + scrollBarSpacing + m_container->style()->pixelMetric(QStyle::PM_ScrollBarExtent, &styleOpt, m_container);
    const int sidePadding = 2 * (2 * option.padding);
    return qMax(ColumnsModeSettings::self()->minColumnWidth(), static_cast<int>(std::ceil(maxWidth)) + sidePadding + chrome);
}

void DolphinColumnPane::setZoomLevel(int level)
{
    m_view->setZoomLevel(level);
}

QUrl DolphinColumnPane::folderUrlFor(const KFileItem &item)
{
    if (item.isNull()) {
        return QUrl();
    }
    // openItemAsFolderUrl() needs the mime type of an archive, which the model determines lazily.
    // Only for a local file, as in openItemAsFolderUrl().
    KFileItem resolved = item;
    if (GeneralSettings::browseThroughArchives() && resolved.isFile() && resolved.targetUrl().isLocalFile() && !resolved.isMimeTypeKnown()) {
        resolved.determineMimeType();
    }
    const QUrl folderUrl = DolphinView::openItemAsFolderUrl(resolved, GeneralSettings::browseThroughArchives());
    if (!folderUrl.isEmpty()) {
        return folderUrl;
    }
    return item.isDir() ? item.url() : QUrl();
}

void DolphinColumnPane::slotItemActivated(int index)
{
    const KFileItem item = m_model->fileItem(index);
    if (item.isNull()) {
        return;
    }
    if (const QUrl folderUrl = folderUrlFor(item); !folderUrl.isEmpty()) {
        Q_EMIT directoryActivated(folderUrl);
    } else {
        Q_EMIT fileActivated(item);
    }
}

void DolphinColumnPane::slotCurrentChanged(int current, int previous)
{
    if (current < 0) {
        // The current item left a list that still has items, as filtering or deleting it does. A
        // reload empties the list instead, and the columns after this one stay.
        if (previous >= 0 && m_model->count() > 0 && !m_activeChildUrl.isEmpty() && m_model->fileItem(m_activeChildUrl).isNull()) {
            m_activeChildUrl.clear();
            Q_EMIT activeChildRemoved();
        }
        return;
    }

    // From no current item, the selection manager made the first item current after the listing.
    // Nobody moved there, so the view must not follow it.
    if (previous < 0 || current >= m_model->count()) {
        return;
    }

    const KFileItem item = m_model->fileItem(current);
    if (!item.isNull()) {
        Q_EMIT currentItemChanged(item);
    }
}

void DolphinColumnPane::slotSelectionChanged()
{
    // Selecting another item clears the selection first, so look once it has settled.
    if (!m_selectionCheckPending) {
        m_selectionCheckPending = true;
        QTimer::singleShot(0, this, &DolphinColumnPane::checkActiveChildSelected);
    }
}

void DolphinColumnPane::checkActiveChildSelected()
{
    m_selectionCheckPending = false;
    if (m_activeChildUrl.isEmpty()) {
        return;
    }
    // Only an empty selection. A press on another item selects it, and the next column stays for a drag to it.
    // A removed item is handled by slotCurrentChanged().
    if (m_controller->selectionManager()->hasSelection() || m_model->fileItem(m_activeChildUrl).isNull()) {
        return;
    }
    m_activeChildUrl.clear();
    Q_EMIT activeChildRemoved();
}

void DolphinColumnPane::reloadSettings()
{
    auto view = itemListView();
    view->readSettings();
    // readSettings() takes both from the details layout. A column never expands a folder.
    view->setSupportsItemExpanding(false);
    view->setHighlightEntireRow(true);
}
