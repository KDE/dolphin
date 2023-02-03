/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_TAB_BAR_H
#define DOLPHIN_TAB_BAR_H

#include <QTabBar>

class DolphinTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit DolphinTabBar(QWidget *parent);

Q_SIGNALS:
    void openNewActivatedTab(int index);
    void tabDropEvent(int index, QDropEvent *event);
    void tabDetachRequested(int index);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    /**
     * Opens a context menu for the tab on the \a event position.
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

private Q_SLOTS:
    void slotAutoActivationTimeout();

private:
    /**
     * If \a index is a valid index (>= 0), store the index and start the timer
     * (if the interval >= 0 ms). If the index is not valid (< 0), stop the timer.
     */
    void updateAutoActivationTimer(const int index);

private:
    QTimer *m_autoActivationTimer;
    int m_autoActivationIndex;
    int m_tabToBeClosedOnMiddleMouseButtonRelease;
};

#endif // DOLPHIN_TAB_BAR_H
