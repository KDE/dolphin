/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINCOLUMNSVIEW_H
#define DOLPHINCOLUMNSVIEW_H

#include "dolphin_export.h"
#include "dolphinview.h"

#include <QPointer>

#include <functional>
#include <optional>
#include <utility>

class DolphinColumnPane;
class QScrollArea;
class QSplitter;
class QTimer;

/**
 * @short Miller columns: one column per folder, from the root to the current folder.
 */
class DOLPHIN_EXPORT DolphinColumnsView : public DolphinView
{
    Q_OBJECT

    friend class DolphinColumnsViewTest;

public:
    /// @p rootUrlResolver names the folder the columns start at, such as the place that holds
    /// the url. Without one they start at the url.
    explicit DolphinColumnsView(const QUrl &url,
                                QWidget *parent,
                                std::optional<Mode> initialMode = std::nullopt,
                                std::function<QUrl(const QUrl &)> rootUrlResolver = {});
    ~DolphinColumnsView() override;

    void setUrl(const QUrl &url) override;
    void setActive(bool active) override;
    void reload() override;
    void stopLoading() override;
    void readSettings() override;

    /// Every column gets the filter, new ones included.
    void setNameFilter(const QString &nameFilter) override;
    void setFilterMode(KFileItemModelFilter::FilterMode mode) override;
    void setFilterCaseSensitive(bool caseSensitive) override;
    KFileItem rootItem() const override;

    int horizontalScrollBarHeight() const override;
    void setStatusBarOffset(int offset) override;
    bool handleSpaceAsNormalKey() const override;
    /// A drag starts from the controller of one of the columns.
    KItemListController *draggingController() const override;
    /// Moving to another column keeps the selection mode.
    bool urlChangeLeavesTheSelectionBehind() const override;

    int columnCount() const;
    DolphinColumnPane *columnAt(int index) const;
    int activeColumnIndex() const;

    void setActiveColumn(int index);

public Q_SLOTS:
    void paste() override;

protected:
    void applyModeToView() override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

    KItemListSelectionManager *activeSelectionManager() const override;
    KFileItemModel *activeModel() const override;
    DolphinItemListView *activeItemListView() const override;

private Q_SLOTS:
    void slotFileActivated(const KFileItem &item);
    /// Marks the folder that the next column shows as soon as it is listed.
    void slotPaneItemsInserted();
    void slotPaneLoadingCompleted();
    void slotSplitterMoved(int pos, int handleIndex);
    void slotActiveSelectionChanged(const KItemSet &current);

private:
    void initColumnsUi();
    void rebuildColumnsForUrl(const QUrl &url);
    /// Scrolls to a column once the splitter has laid out the sizes it was given.
    void scrollToColumnWhenLaidOut(int index);
    /// Gives @p pane its width if it is still waiting for its folder to list.
    void finishPendingWidth(DolphinColumnPane *pane);
    QUrl rootUrlFor(const QUrl &url) const;
    void openChild(int columnIndex, const QUrl &childUrl);
    void popAfter(int columnIndex);
    /// Creates the column for @p dirUrl and adds it after the last one.
    DolphinColumnPane *appendPane(const QUrl &dirUrl);
    DolphinColumnPane *activePane() const;

    void handleKeyLeft(int sourceColumn);
    void handleKeyRight(int sourceColumn);
    bool handleKeyReturn(int sourceColumn);
    /// Opens the column of @p item when it is a folder, or closes the columns after @p column.
    /// Returns whether it was a folder.
    bool followItem(int column, const KFileItem &item);
    /// Closes the columns to the right of \a column, which becomes the url of the view unless the active column is before it.
    void closeColumnsAfter(int column);
    /// Makes the column after @p column active, selecting its first item if nothing is selected.
    void enterChildColumn(int column);
    void handleMouseButtonPressed(DolphinColumnPane *pane, int itemIndex, Qt::MouseButtons buttons);
    void handleMouseButtonReleased(DolphinColumnPane *pane, int itemIndex);

    void ensureActiveColumnVisible();
    void ensureColumnVisible(int index);
    /// Whether the focus is on this view or one of its columns.
    bool hasFocusInside() const;
    /// Scrolls left over the room after the last column, which a wider window leaves.
    void showEarlierColumnsInTheRoomAfterTheLast();
    void autoSelectFirstItem(int columnIndex);
    /// Refit may narrow a column, which only a request by the user does.
    enum class WidthPolicy {
        GrowOnly,
        Refit,
    };
    void recalculateColumnWidths(WidthPolicy policy = WidthPolicy::GrowOnly);
    /// The width that column @p index keeps: what it has now, or the width of the one it replaced.
    int widthFloorFor(int index, const QList<int> &currentSizes) const;
    /// The number of splitter handles drawn before column @p index.
    int visibleHandlesBefore(int index) const;
    void applyColumnSizes(QList<int> columnSizes);
    /// Refits the columns to their content, in the adjust-to-content mode only.
    void refitColumnsToContent();
    /// Fits every column to its content and drops the widths set by hand.
    void autoAdjustColumns();

    /// Shows @p url within the open columns, activating it or opening the folders down to it.
    /// Returns false when @p url is not under an open column.
    bool showUrlInOpenColumns(const QUrl &url, const QUrl &previousUrl = QUrl());

    void syncColumnsFromViewProperties();
    void applyViewProperties(KFileItemModel *model) const;
    void connectActivePane(DolphinColumnPane *newPane);

    QScrollArea *m_scrollArea = nullptr;
    QSplitter *m_splitter = nullptr;
    QWidget *m_filler = nullptr;

    /// A splitter handle does not reliably get a double click, so two releases count as one.
    bool m_splitterReleaseSeen = false;

    int m_statusBarOffset = 0;

    bool m_switchingColumns = false;
    QString m_nameFilter;
    std::function<QUrl(const QUrl &)> m_rootUrlResolver;
    KFileItemModelFilter::FilterMode m_filterMode;
    bool m_filterCaseSensitive = false;

    QList<DolphinColumnPane *> m_columns;
    int m_activeColumn = -1;

    bool m_columnsInitialized = false;
    bool m_blockNavigation = false;

    // A column entered while it still lists, whose first item is selected once it has listed.
    DolphinColumnPane *m_pendingAutoSelect = nullptr;

    // The index and the width of a column that replaced another.
    std::optional<std::pair<int, int>> m_carriedColumnWidth;

    // What a left press landed on, until the release or a drag.
    QPointer<DolphinColumnPane> m_pressedPane;
    int m_pressedItemIndex = -1;
    /// The column that was active before a press moved the focus, which happens before the press arrives.
    QPointer<DolphinColumnPane> m_paneActiveBeforeFocus;
    /// The column that was active before the press, whose item a click to its left deselects.
    QPointer<DolphinColumnPane> m_paneActiveBeforePress;

    // Widths the user dragged, by column index. Not saved.
    QHash<int, int> m_customColumnWidths;

    KFileItemModel *m_baseModel = nullptr;
    QList<QMetaObject::Connection> m_activePaneConnections;
};

#endif // DOLPHINCOLUMNSVIEW_H
