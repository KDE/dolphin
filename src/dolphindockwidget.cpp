/*
 * SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphindockwidget.h"

#include <QEvent>
#include <QStyle>

namespace
{
// Disable the 'Floatable' feature, i.e., the possibility to drag the
// dock widget out of the main window. This works around problems like
// https://bugs.kde.org/show_bug.cgi?id=288629
// https://bugs.kde.org/show_bug.cgi?id=322299
const QDockWidget::DockWidgetFeatures DefaultDockWidgetFeatures = QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable;
}

// Empty titlebar for the dock widgets when "Lock Layout" has been activated.
class DolphinDockTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit DolphinDockTitleBar(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }
    ~DolphinDockTitleBar() override
    {
    }

    QSize minimumSizeHint() const override
    {
        return QSize(0, 0);
    }

    QSize sizeHint() const override
    {
        return minimumSizeHint();
    }
};

DolphinDockWidget::DolphinDockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QDockWidget(title, parent, flags)
    , m_locked(false)
    , m_dockTitleBar(nullptr)
{
    setFeatures(DefaultDockWidgetFeatures);
}

DolphinDockWidget::~DolphinDockWidget()
{
}

void DolphinDockWidget::setLocked(bool lock)
{
    if (lock != m_locked) {
        m_locked = lock;

        if (lock) {
            if (!m_dockTitleBar) {
                m_dockTitleBar = new DolphinDockTitleBar(this);
            }
            setTitleBarWidget(m_dockTitleBar);
            setFeatures(QDockWidget::DockWidgetClosable);
        } else {
            setTitleBarWidget(nullptr);
            setFeatures(DefaultDockWidgetFeatures);
        }
    }
}

bool DolphinDockWidget::isLocked() const
{
    return m_locked;
}

bool DolphinDockWidget::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Show:
    case QEvent::Hide:
        if (event->spontaneous()) {
            // The Dolphin window has been minimized or restored. We do not want this to be interpreted like a user was toggling the visibility of this widget.
            // We return here so no QDockWidget::visibilityChanged() signal is emitted. This does not seem to happen either way on Wayland.
            return true;
        }
        [[fallthrough]];
    default:
        return QDockWidget::event(event);
    }
}

#include "dolphindockwidget.moc"
#include "moc_dolphindockwidget.cpp"
