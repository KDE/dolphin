/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "actiontexthelper.h"

using namespace SelectionMode;

ActionTextHelper::ActionTextHelper(QObject *parent) :
    QObject(parent)
{   }

void ActionTextHelper::registerTextWhenNothingIsSelected(QAction *action, QString registeredText)
{
    Q_CHECK_PTR(action);
    m_registeredActionTextChanges.emplace_back(action, registeredText, TextWhenNothingIsSelected);
}

void ActionTextHelper::textsWhenNothingIsSelectedEnabled(bool enabled)
{
    for (auto i = m_registeredActionTextChanges.begin(); i != m_registeredActionTextChanges.end(); ++i) {
        if (!i->action) {
            i = m_registeredActionTextChanges.erase(i);
            continue;
        }
        if (enabled && i->textStateOfRegisteredText == TextWhenNothingIsSelected) {
            QString textWhenSomethingIsSelected = i->action->text();
            i->action->setText(i->registeredText);
            i->registeredText = textWhenSomethingIsSelected;
            i->textStateOfRegisteredText = TextWhenSomethingIsSelected;
        } else if (!enabled && i->textStateOfRegisteredText == TextWhenSomethingIsSelected) {
            QString textWhenNothingIsSelected = i->action->text();
            i->action->setText(i->registeredText);
            i->registeredText = textWhenNothingIsSelected;
            i->textStateOfRegisteredText = TextWhenNothingIsSelected;
        }
    }
}
