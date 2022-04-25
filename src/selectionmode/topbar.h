/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef SELECTIONMODETOPBAR_H
#define SELECTIONMODETOPBAR_H

#include "global.h"

#include <QPointer>
#include <QPropertyAnimation>
#include <QStyle>
#include <QWidget>

class QHideEvent;
class QLabel;
class QPushButton;
class QResizeEvent;
class QShowEvent;

namespace SelectionMode
{

/**
 * @brief A bar appearing at the top of the view when in selection mode to make users aware of the selection mode state of the application.
 */
class TopBar : public QWidget
{
    Q_OBJECT

public:
    TopBar(QWidget *parent);

    /**
     * Plays a show or hide animation while changing visibility.
     * Therefore, if this method is used to hide this widget, the actual hiding will be postponed until the animation finished.
     * @see QWidget::setVisible()
     */
    void setVisible(bool visible, Animated animated);

Q_SIGNALS:
    void leaveSelectionModeRequested();

protected:
    /** Calls updateLabelString() */
    void resizeEvent(QResizeEvent *resizeEvent) override;

private:
    using QWidget::setVisible; // Makes sure that the setVisible() declaration above doesn't hide the one from QWidget so we can still use it privately.

    /** Decides whether the m_fullLabelString or m_shortLabelString should be used based on available width. */
    void updateLabelString();

private:
    QLabel *m_label;
    QPushButton *m_closeButton;

    /** @see updateLabelString() */
    QString m_fullLabelString;
    /** @see updateLabelString() */
    QString m_shortLabelString;

    int m_preferredHeight;

    QPointer<QPropertyAnimation> m_heightAnimation;
};

}

#endif // SELECTIONMODETOPBAR_H
