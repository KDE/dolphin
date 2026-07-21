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

class DolphinColumnPane;
class QScrollArea;
class QSplitter;
class QTimer;

/**
 * @short Miller Columns view for Dolphin.
 *
 * Inherits DolphinView and adds a column-based directory browsing mode.
 * When the view mode is set to ColumnsView, a horizontal cascade of
 * directory columns is shown (like macOS Finder's column view).
 * For all other view modes, the standard DolphinView behavior is used.
 */
class DOLPHIN_EXPORT DolphinColumnsView : public DolphinView
{
    Q_OBJECT

    friend class DolphinColumnsViewTest;

public:
    explicit DolphinColumnsView(const QUrl &url, QWidget *parent, std::optional<Mode> initialMode = std::nullopt);
    ~DolphinColumnsView() override;

    // --- DolphinView overrides ---
    void setUrl(const QUrl &url) override;
    void setActive(bool active) override;
    void reload() override;
    void stopLoading() override;
    void readSettings() override;
    KFileItem rootItem() const override;

    // --- Columns-specific API ---
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
    void slotColumnsCurrentItemChanged(const KFileItem &item);
    void slotPaneLoadingCompleted();
    void slotSplitterMoved(int pos, int handleIndex);

private:
    void initColumnsUi();
    void rebuildColumnsForUrl(const QUrl &url);
    void openChild(int columnIndex, const QUrl &childUrl);
    void popAfter(int columnIndex);
    DolphinColumnPane *createPane(const QUrl &dirUrl);
    DolphinColumnPane *activePane() const;

    /**
     * Returns the URL of the folder that @p item navigates into (a directory,
     * or e.g. an archive when browsing those is supported), or an empty URL if
     * @p item is not a navigable folder.
     */
    QUrl folderUrlForItem(const KFileItem &item) const;

    void handleKeyLeft(int sourceColumn);
    void handleKeyRight(int sourceColumn);
    bool handleKeyReturn(int sourceColumn);
    void handleMouseButtonPressed(DolphinColumnPane *pane, int itemIndex, Qt::MouseButtons buttons);

    void ensureActiveColumnVisible();
    void autoSelectFirstItem(int columnIndex);
    void recalculateColumnWidths();
    /// Set the per-column widths on the splitter and update the horizontal-scroll overflow.
    void applyColumnSizes(QList<int> columnSizes);
    /**
     * In "adjust to content" mode, re-fit the columns to their content so a rename
     * or an added/removed item resizes the column. A no-op in fixed-width mode.
     * Columns the user sized by hand keep their width.
     */
    void refitColumnsToContent();
    /**
     * Fit every column to its content, dropping any width the user set by hand.
     * Triggered by a double-click on a splitter handle.
     */
    void autoAdjustColumns();

    void syncColumnsFromViewProperties();
    void reconnectActivePane(DolphinColumnPane *oldPane, DolphinColumnPane *newPane);

    QScrollArea *m_scrollArea = nullptr;
    QSplitter *m_splitter = nullptr;
    QWidget *m_filler = nullptr;

    QList<DolphinColumnPane *> m_columns;
    int m_activeColumn = -1;

    bool m_columnsInitialized = false;
    bool m_blockNavigation = false;

    // Set when Right-arrow opens a new child column whose model is still
    // loading.  slotPaneLoadingCompleted() uses this to auto-select the
    // first item once the model is ready.
    DolphinColumnPane *m_pendingAutoSelect = nullptr;

    // Per-column custom width set by user dragging splitter handles.
    // Key = column index, value = width in pixels. Session-only, not persisted.
    QHash<int, int> m_customColumnWidths;

    KFileItemModel *m_baseModel = nullptr;
    QTimer *m_columnsSelectionTimer = nullptr;
};

#endif // DOLPHINCOLUMNSVIEW_H
