/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2020 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DOLPHINURLNAVIGATORSCONTROLLER_H
#define DOLPHINURLNAVIGATORSCONTROLLER_H

#include <KCompletion>

#include <QObject>

#include <forward_list>

class DolphinUrlNavigator;

/**
 * @brief A controller managing all DolphinUrlNavigators.
 *
 * This class is used to apply settings changes to all constructed DolphinUrlNavigators.
 *
 * @see DolphinUrlNavigator
 */
class DolphinUrlNavigatorsController : public QObject
{
    Q_OBJECT

public:
    DolphinUrlNavigatorsController() = delete;

public Q_SLOTS:
    /**
     * Refreshes all DolphinUrlNavigators to get synchronized with the
     * Dolphin settings if they were changed.
     */
    static void slotReadSettings();

    static void slotPlacesPanelVisibilityChanged(bool visible);

private:
    /**
     * @return wether the places selector of DolphinUrlNavigators should be visible.
     */
    static bool placesSelectorVisible();

    /**
     * Adds \p dolphinUrlNavigator to the list of DolphinUrlNavigators
     * controlled by this class.
     */
    static void registerDolphinUrlNavigator(DolphinUrlNavigator *dolphinUrlNavigator);

    /**
     * Removes \p dolphinUrlNavigator from the list of DolphinUrlNavigators
     * controlled by this class.
     */
    static void unregisterDolphinUrlNavigator(DolphinUrlNavigator *dolphinUrlNavigator);

private Q_SLOTS:
    /**
     * Sets the completion mode for all DolphinUrlNavigators and saves it in settings.
     */
    static void setCompletionMode(const KCompletion::CompletionMode completionMode);

private:
    /** Contains all currently constructed DolphinUrlNavigators */
    static std::forward_list<DolphinUrlNavigator *> s_instances;

    /** Caches the (negated) places panel visibility */
    static bool s_placesSelectorVisible;

    friend class DolphinUrlNavigator;
};

#endif // DOLPHINURLNAVIGATORSCONTROLLER_H
