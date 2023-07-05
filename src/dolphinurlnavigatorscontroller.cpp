/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2020 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "dolphinurlnavigatorscontroller.h"

#include "dolphin_generalsettings.h"
#include "dolphinurlnavigator.h"
#include "global.h"

#include <KUrlComboBox>

void DolphinUrlNavigatorsController::slotReadSettings()
{
    // The startup settings should (only) get applied if they have been
    // modified by the user. Otherwise keep the (possibly) different current
    // settings of the URL navigators and split view.
    if (GeneralSettings::modifiedStartupSettings()) {
        for (DolphinUrlNavigator *urlNavigator : s_instances) {
            urlNavigator->setUrlEditable(GeneralSettings::editableUrl());
            urlNavigator->setShowFullPath(GeneralSettings::showFullPath());
            urlNavigator->setHomeUrl(Dolphin::homeUrl());
        }
    }
}

void DolphinUrlNavigatorsController::slotPlacesPanelVisibilityChanged(bool visible)
{
    // The places-selector from the URL navigator should only be shown
    // if the places dock is invisible
    s_placesSelectorVisible = !visible;

    for (DolphinUrlNavigator *urlNavigator : s_instances) {
        urlNavigator->setPlacesSelectorVisible(s_placesSelectorVisible);
    }
}

bool DolphinUrlNavigatorsController::placesSelectorVisible()
{
    return s_placesSelectorVisible;
}

void DolphinUrlNavigatorsController::registerDolphinUrlNavigator(DolphinUrlNavigator *dolphinUrlNavigator)
{
    s_instances.push_front(dolphinUrlNavigator);
    connect(dolphinUrlNavigator->editor(), &KUrlComboBox::completionModeChanged, DolphinUrlNavigatorsController::setCompletionMode);
}

void DolphinUrlNavigatorsController::unregisterDolphinUrlNavigator(DolphinUrlNavigator *dolphinUrlNavigator)
{
    s_instances.remove(dolphinUrlNavigator);
}

void DolphinUrlNavigatorsController::setCompletionMode(const KCompletion::CompletionMode completionMode)
{
    if (completionMode != GeneralSettings::urlCompletionMode()) {
        GeneralSettings::setUrlCompletionMode(completionMode);
        for (const DolphinUrlNavigator *urlNavigator : s_instances) {
            urlNavigator->editor()->setCompletionMode(completionMode);
        }
    }
}

std::forward_list<DolphinUrlNavigator *> DolphinUrlNavigatorsController::s_instances;
bool DolphinUrlNavigatorsController::s_placesSelectorVisible = true;

#include "moc_dolphinurlnavigatorscontroller.cpp"
