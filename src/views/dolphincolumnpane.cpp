/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphincolumnpane.h"

#include "dolphin_columnsmodesettings.h"
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
#include <QVBoxLayout>
#include <cmath>

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

    // The full-row selection highlight is drawn styleOption().padding wider than
    // the row on each side. Give the list matching side padding so the highlight
    // stays inside the pane instead of spilling past the right edge.
    const int sidePadding = 2 * m_view->styleOption().padding;
    m_view->header()->setSidePadding(sidePadding, sidePadding);

    // Controller takes ownership of the model.
    m_controller = new KItemListController(m_model, m_view, this);
    m_controller->setSelectionBehavior(KItemListController::MultiSelection);

    m_container = new KItemListContainer(m_controller, this);
    m_container->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_view->setAccessibleParentsObject(m_container);

    layout->addWidget(m_container);

    // VCS plugin support: model signals (itemsInserted, directoryLoadingCompleted)
    // drive automatic version detection; setView() is skipped because it
    // requires a DolphinView* but is only used for re-verification on activation.
    m_versionControlObserver = new VersionControlObserver(this);
    m_versionControlObserver->setModel(m_model);
    connect(m_versionControlObserver, &VersionControlObserver::infoMessage, this, &DolphinColumnPane::infoMessage);
    connect(m_versionControlObserver, &VersionControlObserver::errorMessage, this, [this](const QString &msg) {
        Q_EMIT errorMessage(msg);
    });
    connect(m_versionControlObserver, &VersionControlObserver::operationCompletedMessage, this, &DolphinColumnPane::operationCompletedMessage);

    setMinimumWidth(100);

    // Connect signals
    connect(m_controller, &KItemListController::itemActivated, this, &DolphinColumnPane::slotItemActivated);
    connect(m_controller->selectionManager(), &KItemListSelectionManager::currentChanged, this, &DolphinColumnPane::slotCurrentChanged);
    connect(m_model, &KFileItemModel::directoryLoadingCompleted, this, &DolphinColumnPane::directoryLoadingCompleted);
}

DolphinColumnPane::~DolphinColumnPane() = default;

QUrl DolphinColumnPane::dirUrl() const
{
    return m_model->directory();
}

void DolphinColumnPane::setActiveChildUrl(const QUrl &childUrl)
{
    if (childUrl.isEmpty()) {
        clearActiveChild();
        return;
    }

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

    // A hidden name takes part in the width exactly while the model shows hidden
    // files, so the column has room for every name on display.
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

    // Reserve the horizontal chrome the container subtracts from the content area
    // (frame + scrollbar spacing + scrollbar extent), always including the scrollbar
    // even while it is hidden. This keeps the column width stable as the bar toggles
    // and guarantees the row - and thus its full-row selection highlight - never ends
    // up wider than the viewport (which would draw it under the scrollbar).
    QStyleOption styleOpt;
    styleOpt.initFrom(m_container);
    int scrollBarSpacing = 0;
    if (m_container->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &styleOpt, m_container)) {
        scrollBarSpacing = m_container->style()->pixelMetric(QStyle::PM_ScrollView_ScrollBarSpacing, &styleOpt, m_container);
    }
    const int chrome = m_container->frameWidth() * 2 + scrollBarSpacing + m_container->style()->pixelMetric(QStyle::PM_ScrollBarExtent, &styleOpt, m_container);
    // Side padding (set in the constructor) reserved on both sides so the widest
    // entry is not truncated by it.
    const int sidePadding = 2 * (2 * option.padding);
    return qMax(ColumnsModeSettings::self()->minColumnWidth(), static_cast<int>(std::ceil(maxWidth)) + sidePadding + chrome);
}

void DolphinColumnPane::setZoomLevel(int level)
{
    m_view->setZoomLevel(level);
}

void DolphinColumnPane::slotItemActivated(int index)
{
    if (index < 0 || index >= m_model->count()) {
        return;
    }

    const KFileItem item = m_model->fileItem(index);
    if (item.isNull()) {
        return;
    }

    // itemActivated = double-click (or single-click if system configured so).
    // Directories: navigate.  Files: open with default application.
    const QUrl folderUrl = DolphinView::openItemAsFolderUrl(item, false);
    if (!folderUrl.isEmpty()) {
        Q_EMIT directoryActivated(folderUrl);
    } else if (item.isDir()) {
        Q_EMIT directoryActivated(item.url());
    } else {
        Q_EMIT fileActivated(item);
    }
}

void DolphinColumnPane::slotCurrentChanged(int current, int previous)
{
    Q_UNUSED(previous)

    if (current < 0 || current >= m_model->count()) {
        return;
    }

    const KFileItem item = m_model->fileItem(current);
    if (!item.isNull()) {
        Q_EMIT currentItemChanged(item);
    }
}
