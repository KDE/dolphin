/*
 * SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphindockwidget.h"

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
        const int border = style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin);
        return QSize(border, border);
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
            setFeatures(QDockWidget::NoDockWidgetFeatures);
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

#include "dolphindockwidget.moc"
