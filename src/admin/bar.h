/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef ADMINBAR_H
#define ADMINBAR_H

#include "animatedheightwidget.h"

class DolphinViewContainer;
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
    explicit Bar(DolphinViewContainer *parentViewContainer);

    /** Used to recolor this bar when this application's color scheme changes. */
    bool event(QEvent *event) override;

protected:
    /** Calls updateLabelString() */
    void resizeEvent(QResizeEvent *resizeEvent) override;

private:
    /**
     * Makes sure this admin bar hides itself when the elevated privileges expire so the user doesn't mistakenly assume that they are still acting with
     * administrative rights. The view container is also changed to a non-admin url, so no password prompts will pop up unexpectedly.
     * Then this method shows a message to the user to explain this.
     * The mechanism of this method only fires once and will need to be called again the next time the user regains administrative rights for this view.
     */
    void hideTheNextTimeAuthorizationExpires();

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

    /**
     * A proxy action for the real actAsAdminAction.
     * This proxy action can be used to reenable admin mode for the view belonging to this bar object specifically.
     */
    QAction *m_reenableActAsAdminAction = nullptr;

    /**
     * The parent of this bar. The bar acts on the DolphinViewContainer to exit the admin mode. This can happen in two ways:
     * 1. The user closes the bar which implies exiting of the admin mode.
     * 2. The admin authorization expires so all views can factually no longer be in admin mode.
     */
    DolphinViewContainer *m_parentViewContainer;
};

}

#endif //  ADMINBAR_H
