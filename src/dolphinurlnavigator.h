/*
 * Copyright 2020  Felix Ernst <fe.a.ernst@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DOLPHINURLNAVIGATOR_H
#define DOLPHINURLNAVIGATOR_H

#include <KCompletion>
#include <KUrlNavigator>

#include <forward_list>

class KToggleAction;

/**
 * @brief Extends KUrlNavigator in a Dolphin-specific way
 * 
 * Makes sure that Dolphin preferences, settings and settings changes are
 * applied to all constructed DolphinUrlNavigators.
 * 
 * @see KUrlNavigator
 */
class DolphinUrlNavigator : public KUrlNavigator
{
    Q_OBJECT

public:
    /**
     * Applies all Dolphin-specific settings to a KUrlNavigator
     * @see KUrlNavigator::KurlNavigator()
     */
    DolphinUrlNavigator(QWidget *parent = nullptr);

    /**
     * Applies all Dolphin-specific settings to a KUrlNavigator
     * @see KUrlNavigator::KurlNavigator()
     */
    DolphinUrlNavigator(const QUrl &url, QWidget *parent = nullptr);

    virtual ~DolphinUrlNavigator();

public slots:
    /**
     * Refreshes all DolphinUrlNavigators to get synchronized with the
     * Dolphin settings if they were changed.
     */
    static void slotReadSettings();

    /**
     * Switches to "breadcrumb" mode if the editable mode is not set to be
     * preferred in the Dolphin settings.
     */
    void slotReturnPressed();

    /**
     * This method is specifically here so the locationInToolbar
     * KToggleAction that is created in DolphinMainWindow can be passed to
     * this class and then appear in all context menus. This last part is
     * done by eventFilter().
     * For any other use parts of this class need to be rewritten.
     * @param action The locationInToolbar-action from DolphinMainWindow
     */
    static void addToContextMenu(QAction *action);

    static void slotPlacesPanelVisibilityChanged(bool visible);

protected:
    /**
     * Constructor-helper function
     */
    void init();

    /**
     * This filter adds the s_ActionForContextMenu action to QMenus which
     * are spawned by the watched QObject if that QMenu contains at least
     * two separators.
     * @see addToContextMenu()
     */
    bool eventFilter(QObject * watched, QEvent * event) override;

protected slots:
    /**
     * Sets the completion mode for all DolphinUrlNavigators
     * and saves it in settings.
     */
    static void setCompletionMode(const KCompletion::CompletionMode completionMode);

protected:
    /** Contains all currently constructed DolphinUrlNavigators */
    static std::forward_list<DolphinUrlNavigator *> s_instances;

    /** Caches the (negated) places panel visibility */
    static bool s_placesSelectorVisible;

    /** An action that is added to the context menu */
    static QAction *s_ActionForContextMenu;
};

#endif // DOLPHINURLNAVIGATOR_H
