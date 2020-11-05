/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2020 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DOLPHINNAVIGATORSWIDGETACTION_H
#define DOLPHINNAVIGATORSWIDGETACTION_H

#include "dolphinurlnavigator.h"

#include <QSplitter>
#include <QTimer>
#include <QWidgetAction>

#include <memory>

class KXmlGuiWindow;
class QPushButton;

/**
 * @brief QWidgetAction that allows to use DolphinUrlNavigators in a toolbar.
 *
 * This class is mainly a container that manages up to two DolphinUrlNavigator objects so they
 * can be added to a toolbar. It also deals with alignment.
 *
 * The structure of the defaultWidget() of this QWidgetAction is as follows:
 * - A QSplitter manages up to two sides which each correspond to one DolphinViewContainer.
 *      The secondary side only exists for split view and is created by
 *      createSecondaryUrlNavigator() when necessary.
 * - Each side is a QWidget which I call NavigatorWidget with a QHBoxLayout.
 * - Each NavigatorWidget consists an UrlNavigator, an emptyTrashButton and spacing.
 * - Only the primary navigatorWidget has leading spacing. Both have trailing spacing.
 *      The spacing is there to align the UrlNavigator with its DolphinViewContainer.
 */
class DolphinNavigatorsWidgetAction : public QWidgetAction
{
    Q_OBJECT

public:
    DolphinNavigatorsWidgetAction(QWidget *parent = nullptr);

    /**
     * Adds this action to the mainWindow's toolbar and saves the change
     * in the users ui configuration file.
     * @return true if successful. Otherwise false.
     */
    bool addToToolbarAndSave(KXmlGuiWindow *mainWindow);

    /**
     * The secondary UrlNavigator is only created on-demand. Such an action is not necessary
     * for the primary UrlNavigator which is created preemptively.
     *
     * This method should preferably only be called when:
     * - Split view is activated in the active tab
     * OR
     * - A switch to a tab that is already in split view mode is occuring
     */
    void createSecondaryUrlNavigator();

    /**
     * Notify the primary UrlNavigator of changes in geometry of the ViewContainer it tries to be
     * aligned with. Only call this method if there is no secondary UrlNavigator.
     */
    void followViewContainerGeometry(int globalXOfPrimary,   int widthOfPrimary);
    /**
     * Notify this widget of changes in geometry of the ViewContainers it tries to be
     * aligned with.
     */
    void followViewContainersGeometry(int globalXOfPrimary,   int widthOfPrimary,
                                      int globalXOfSecondary, int widthOfSecondary);

    /**
     * @return the primary UrlNavigator.
     */
    DolphinUrlNavigator *primaryUrlNavigator() const;
    /**
     * @return the secondary UrlNavigator and nullptr if it doesn't exist.
     */
    DolphinUrlNavigator *secondaryUrlNavigator() const;

    /**
     * Change the visibility of the secondary UrlNavigator including spacing.
     * @param visible Setting this to false will completely hide the secondary side of this
     *                WidgetAction's QSplitter making the QSplitter effectively disappear.
     */
    void setSecondaryNavigatorVisible(bool visible);

private:
    /**
     * Adjusts the width of the spacings used to align the UrlNavigators with ViewContainers.
     * This can only work nicely if up-to-date geometry of ViewContainers is cached so
     * followViewContainersGeometry() has to have been called at least once before.
     */
    void adjustSpacing();

    /**
     * In Left-to-right languages the Primary side will be the left one.
     */
    enum Side {
        Primary,
        Secondary
    };
    /**
     * Used to create the navigatorWidgets for both sides of the QSplitter.
     */
    QWidget *createNavigatorWidget(Side side) const;

    /**
     * Used to retrieve the emptyTrashButtons for the navigatorWidgets on both sides.
     */
    QPushButton *emptyTrashButton(Side side);

    /**
     * Creates a new empty trash button.
     * @param urlNavigator Only when this UrlNavigator shows the trash directory
     *                     will the the button be visible.
     * @param parent       Aside from the usual QObject deletion mechanisms,
     *                     this parameter influences the positioning of dialog windows
     *                     pertaining to this trash button.
     */
    QPushButton *newEmptyTrashButton(const DolphinUrlNavigator *urlNavigator, QWidget *parent) const;

    enum Position {
        Leading,
        Trailing
    };
    /**
     * Used to retrieve both the leading and trailing spacing for the navigatorWidgets
     * on both sides. A secondary leading spacing does not exist.
     */
    QWidget *spacing(Side side, Position position) const;

    /**
     * Sets this action's text depending on the amount of visible UrlNavigators.
     */
    void updateText();

    /**
     * The defaultWidget() of this QWidgetAction.
     */
    std::unique_ptr<QSplitter> m_splitter;

    /**
     * adjustSpacing() has to be called slightly later than when urlChanged is emitted.
     * This timer bridges that time.
     */
    std::unique_ptr<QTimer> m_adjustSpacingTimer;

    // cached values
    int m_globalXOfSplitter;
    int m_globalXOfPrimary;
    int m_widthOfPrimary;
    int m_globalXOfSecondary;
    int m_widthOfSecondary;
};

#endif // DOLPHINNAVIGATORSWIDGETACTION_H
