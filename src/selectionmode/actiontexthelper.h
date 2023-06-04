/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef ACTIONTEXTHELPER_H
#define ACTIONTEXTHELPER_H

#include <QAction>
#include <QPointer>
#include <QString>

namespace SelectionMode
{

/**
 * @brief Helps changing the texts of actions depending on the current selection.
 *
 * This is useful for actions that directly trigger a change when there is a selection and do something
 * different when nothing is selected. For example should the copy action read "Copy" when items are
 * selected but when no items are selected it can read "Copy…" since triggering it will enter selection
 * mode and ask users to select the files they want to copy first.
 */
class ActionTextHelper : QObject
{
public:
    explicit ActionTextHelper(QObject *parent);

    /**
     * Changes the text of \a action to \a text whenever textsWhenNothingIsSelectedEnabled(true) is called.
     * The texts can be changed back by calling textsWhenNothingIsSelectedEnabled(false).
     * @see textsWhenNothingIsSelectedEnabled()
     */
    void registerTextWhenNothingIsSelected(QAction *action, QString registeredText);

    /**
     * Changes all texts that were registered previously using registerTextWhenNothingIsSelected() to those
     * registered texts if called with \a enabled == true. Otherwise resets the texts to the original one.
     */
    void textsWhenNothingIsSelectedEnabled(bool enabled);

private:
    enum TextState { TextWhenNothingIsSelected, TextWhenSomethingIsSelected };

    /**
     * Utility struct to allow switching back and forth between registered actions showing their
     * distinct texts for when no items are selected or when items are selected.
     * An example is "Copy" or "Copy…". The latter one is used when nothing is selected and signifies
     * that it will trigger SelectionMode so items can be selected and then copied.
     */
    struct RegisteredActionTextChange {
        QPointer<QAction> action;
        QString registeredText;
        TextState textStateOfRegisteredText;

        RegisteredActionTextChange(QAction *action, QString registeredText, TextState state)
            : action{action}
            , registeredText{registeredText}
            , textStateOfRegisteredText{state} {};
    };

    /**
     * @see RegisteredActionTextChange
     */
    std::vector<RegisteredActionTextChange> m_registeredActionTextChanges;
};

}

#endif // ACTIONTEXTHELPER_H
