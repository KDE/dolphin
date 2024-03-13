/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef SELECTIONMODETOPBAR_H
#define SELECTIONMODETOPBAR_H

#include "animatedheightwidget.h"

class QLabel;
class QPushButton;
class QResizeEvent;

namespace SelectionMode
{

/**
 * @brief A bar appearing at the top of the view when in selection mode to make users aware of the selection mode state of the application.
 */
class TopBar : public AnimatedHeightWidget
{
    Q_OBJECT

public:
    TopBar(QWidget *parent);

Q_SIGNALS:
    void selectionModeLeavingRequested();

protected:
    /** Calls updateLabelString() */
    void resizeEvent(QResizeEvent *resizeEvent) override;

private:
    /** Decides whether the m_fullLabelString or m_shortLabelString should be used based on available width. */
    void updateLabelString();

    /** @see AnimatedHeightWidget::preferredHeight() */
    inline int preferredHeight() const override
    {
        return m_preferredHeight;
    };

private:
    QLabel *m_label;
    QPushButton *m_closeButton;

    /** @see updateLabelString() */
    QString m_fullLabelString;
    /** @see updateLabelString() */
    QString m_shortLabelString;

    int m_preferredHeight;
};

}

#endif // SELECTIONMODETOPBAR_H
