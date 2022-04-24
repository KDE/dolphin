/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef ACTIONWITHWIDGET_H
#define ACTIONWITHWIDGET_H

#include <QAction>
#include <QPointer>
#include <QWidget>

class QAbstractButton;

namespace SelectionMode
{

/**
 * @brief Small wrapper/helper class that contains an action and its widget.
 *
 * This class takes neither the responsibility for deleting its action() nor its widget().
 */
class ActionWithWidget
{
public:
    ActionWithWidget(QAction *action);

    /**
     * Connect @p action and @p button using copyActionDataToButton() and the
     * wraps the two together in the ActionWithWidget object.
     * ActionWithWidget doesn't take any ownership.
     *
     * @see copyActionDataToButton()
     *
     * @param button the button to be styled and used to fit the @p action.
     */
    ActionWithWidget(QAction *action, QAbstractButton *button);

    /** @returns the action of this object. Crashes if that action has been deleted elsewhere in the meantime. */
    inline QAction *action() {
        Q_CHECK_PTR(m_action);
        return m_action;
    };

    /** @returns the widget of this object. */
    inline QWidget *widget() {
        return m_widget;
    }

    /**
     * @returns a widget with parent @p parent for the action() of this object.
     *
     * For most actions some sort of button will be returned. For separators a vertical line will be returned.
     * If this ActionWithWidget already has a widget(), this method will crash.
     */
    QWidget *newWidget(QWidget *parent);

    /** returns true if the widget exists and is visible. false otherwise. */
    inline bool isWidgetVisible() const {
        return m_widget && m_widget->isVisible();
    };

private:
    QPointer<QAction> m_action;
    QPointer<QWidget> m_widget;
};

/**
 * A small helper method.
 * @return a button with the correct styling for the general mode of the SelectionModeBottomBar which can be added to its layout.
 */
QAbstractButton *newButtonForAction(QAction *action, QWidget *parent);

/**
 * Normally, if one wants a button that represents a QAction one would use a QToolButton
 * and simply call QToolButton::setDefaultAction(action). However if one does this, all
 * control over the style, text, etc. of the button is forfeited. One can't for example
 * have text on the button then, if the action has a low QAction::priority().
 *
 * This method styles the @p button based on the @p action without using QToolButton::setDefaultAction().
 *
 * Another reason why this is necessary is because the actions have application-wide scope while
 * these buttons belong to one ViewContainer.
 */
void copyActionDataToButton(QAbstractButton *button, QAction *action);

}

#endif // ACTIONWITHWIDGET_H
