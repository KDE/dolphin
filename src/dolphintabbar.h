/***************************************************************************
 * Copyright (C) 2013 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
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

#ifndef DOLPHIN_TAB_BAR_H
#define DOLPHIN_TAB_BAR_H

#include <QTabBar>

class QLabel;
class KUrl;

class DolphinTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit DolphinTabBar(QWidget* parent);

    /**
     * Sets the delay in milliseconds when dragging an object above a tab
     * until the tab gets activated automatically. A value of -1 indicates
     * that no automatic activation will be done at all (= default value).
     */
    void setAutoActivationDelay(int delay);
    int autoActivationDelay() const;

signals:
    void openNewActivatedTab();
    void openNewActivatedTab(int index);
    void openNewActivatedTab(const KUrl& url);
    void tabDropEvent(int index, QDropEvent* event);
    void tabDetachRequested(int index);

protected:
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

    /**
     * Opens a context menu for the tab on the \a event position.
     */
    virtual void contextMenuEvent(QContextMenuEvent* event);

private slots:
    void slotAutoActivationTimeout();

private:
    /**
     * If \a index is a valid index (>= 0), store the index and start the timer (if the
     * interval >= 0ms) afterwards. If the index is not valid (< 0), stop the timer.
     */
    void updateAutoActivationTimer(const int index);

private:
    QTimer* m_autoActivationTimer;
};

#endif