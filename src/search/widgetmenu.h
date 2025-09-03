/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef WIDGETMENU_H
#define WIDGETMENU_H

#include <QMenu>

class QMouseEvent;
class QShowEvent;

namespace Search
{

/**
 * @brief A QMenu that contains nothing but a lazily constructed widget.
 *
 * Usually QMenus contain a list of actions. WidgetMenu allows showing any QWidget instead. This is useful to show popups, random text, or full user interfaces
 * when a button is pressed or a menu action in a QMenu is hovered.
 *
 * This class also encapsulates lazy construction of the widget within. It will only be created when this menu is actually being opened.
 */
class WidgetMenu : public QMenu
{
public:
    explicit WidgetMenu(QWidget *parent = nullptr);

protected:
    /**
     * Overrides the weird QMenu Tab key handling with the usual QWidget one.
     */
    bool focusNextPrevChild(bool next) override;

    /**
     * Overrides the QMenu behaviour of closing itself when clicked with the non-closing QWidget one.
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * This unfortunately needs to be explicitly called to resize the WidgetMenu because the size of a QMenu will not automatically change to fit the QWidgets
     * within.
     */
    void resizeToFitContents();

    /**
     * Move focus to the widget when this WidgetMenu is shown.
     */
    void showEvent(QShowEvent *event) override;

private:
    /**
     * @return the widget which is contained in this WidgetMenu. This method is at most called once per WidgetMenu object when the WidgetMenu is about to be
     *         shown for the first time. The ownership of the widget will be transferred to an internal QWidgetAction.
     */
    virtual QWidget *init() = 0;
};

}

#endif // WIDGETMENU_H
