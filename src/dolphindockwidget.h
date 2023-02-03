/*
 * SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_DOCK_WIDGET_H
#define DOLPHIN_DOCK_WIDGET_H

#include <QDockWidget>

/**
 * @brief Extends QDockWidget to be able to get locked.
 */
class DolphinDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit DolphinDockWidget(const QString &title = QString(), QWidget *parent = nullptr, Qt::WindowFlags flags = {});
    ~DolphinDockWidget() override;

    /**
     * @param lock If \a lock is true, the title bar of the dock-widget will get hidden so
     *             that it is not possible for the user anymore to move or undock the dock-widget.
     */
    void setLocked(bool lock);
    bool isLocked() const;

private:
    bool m_locked;
    QWidget *m_dockTitleBar;
};

#endif
