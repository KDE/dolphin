/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef ADMINBAR_H
#define ADMINBAR_H

#include "animatedheightwidget.h"

class QLabel;
class QPushButton;
class QResizeEvent;
class QToolButton;

namespace Admin
{

/**
 * @brief A bar appearing above the view while the user is acting with administrative privileges.
 *
 * It contains a warning and allows revoking this administrative mode by closing this bar.
 */
class Bar : public AnimatedHeightWidget
{
    Q_OBJECT

public:
    explicit Bar(QWidget *parent);

    /** Used to recolor this bar when this application's color scheme changes. */
    bool event(QEvent *event) override;

Q_SIGNALS:
    void activated();

protected:
    /** Calls updateLabelString() */
    void resizeEvent(QResizeEvent *resizeEvent) override;

private:
    /** Recolors this bar based on the current color scheme. */
    void updateColors();

    /** Decides whether the m_fullLabelString or m_shortLabelString should be used based on available width. */
    void updateLabelString();

    /** @see AnimatedHeightWidget::preferredHeight() */
    inline int preferredHeight() const override
    {
        return m_preferredHeight;
    };

private:
    /** The text on this bar */
    QLabel *m_label = nullptr;
    /** Shows a warning about the dangers of acting with administrative privileges. */
    QToolButton *m_warningButton = nullptr;
    /** Closes this bar and exits the administrative mode. */
    QPushButton *m_closeButton = nullptr;

    /** @see updateLabelString() */
    QString m_fullLabelString;
    /** @see updateLabelString() */
    QString m_shortLabelString;

    /** @see preferredHeight() */
    int m_preferredHeight;
};

}

#endif //  ADMINBAR_H
