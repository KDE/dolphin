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

    void setNewTabButtonVisible(bool visible);
Q_SIGNALS:
    void openNewActivatedTab(int index);
    void tabDragMoveEvent(int index, QDragMoveEvent *event);
    void tabDropEvent(int index, QDropEvent *event);
    void tabDetachRequested(int index);
    void tabRenamed(int index, const QString &label);
    void newTabRequested();

protected:
    QSize tabSizeHint(int index) const override;
    QSize minimumSizeHint() const override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void tabLayoutChange() override;
    void tabInserted(int index) override;
    void tabRemoved(int index) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    /**
     * Opens a context menu for the tab on the \a event position.
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

private Q_SLOTS:
    void slotAutoActivationTimeout();
    void slotTabBarChanged();

private:
    /**
     * If \a index is a valid index (>= 0), store the index and start the timer
     * (if the interval >= 0 ms). If the index is not valid (< 0), stop the timer.
     */
    void updateAutoActivationTimer(const int index);
    void updateNewTabButtonGeometry();

private:
    QTimer *m_autoActivationTimer;
    int m_autoActivationIndex;
    int m_tabToBeClosedOnMiddleMouseButtonRelease = -1;
    QToolButton *m_newTabButton = nullptr;
};

#endif // DOLPHIN_TAB_BAR_H
