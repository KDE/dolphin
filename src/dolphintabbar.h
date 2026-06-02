/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_TAB_BAR_H
#define DOLPHIN_TAB_BAR_H

#include <QTabBar>

class QToolButton;

class DolphinTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit DolphinTabBar(QWidget *parent);

    bool isPinned(int index) const;
    void setPinned(int index, bool pinned);
    int pinnedCount() const;

    void setNewTabButtonVisible(bool visible);

Q_SIGNALS:
    void openNewActivatedTab(int index);
    void tabDragMoveEvent(int index, QDragMoveEvent *event);
    void tabDropEvent(int index, QDropEvent *event);
    void tabDetachRequested(int index);
    void tabRenamed(int index, const QString &label);
    void newTabRequested();
    void tabPinStateChanged(int index, bool pinned);

protected:
    QSize tabSizeHint(int index) const override;
    QSize minimumSizeHint() const override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void tabInserted(int index) override;
    void tabRemoved(int index) override;
    void tabLayoutChange() override;

    /**
     * Opens a context menu for the tab on the \a event position.
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

private Q_SLOTS:
    void slotAutoActivationTimeout();
    void slotTabBarChanged();

private:
    void updateAutoActivationTimer(const int index);
    void updateNewTabButtonGeometry();
    void updatePinnedTabButtons();

    bool isOutsideUndetachableZone(const QPoint &pos) const;
    void updateTabDetachDrag(QMouseEvent *event);
    void startTabDrag(QMouseEvent *event);
    void cancelTabDrag();

private:
    struct DragState {
        int tabIndex = -1;
        QPoint startPosition;
        int offsetInTabX = 0;
        int offsetInTabY = 0;
        int tabWidth = 0;
        int tabHeight = 0;
        bool isOngoing = false;
    };

    QTimer *m_autoActivationTimer;
    int m_autoActivationIndex;
    int m_tabToBeClosedOnMiddleMouseButtonRelease = -1;
    QToolButton *m_newTabButton = nullptr;
    DragState m_drag;
    int m_pressedTab = -1;
    int m_dragBoundaryX = -1;
    int m_clickOffsetInTab = 0;
};

#endif // DOLPHIN_TAB_BAR_H
