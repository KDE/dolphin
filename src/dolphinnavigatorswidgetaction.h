/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2020 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DOLPHINNAVIGATORSWIDGETACTION_H
#define DOLPHINNAVIGATORSWIDGETACTION_H

#include "dolphinurlnavigator.h"

#include <QPointer>
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
     * Adjusts the width of the spacings used to align the UrlNavigators with ViewContainers.
     * This can only work nicely if up-to-date geometry of ViewContainers is cached so
     * followViewContainersGeometry() has to have been called at least once before.
     */
    void adjustSpacing();

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
     * Notify this widget of changes in geometry of the ViewContainers it tries to be
     * aligned with.
     */
    void followViewContainersGeometry(QWidget *primaryViewContainer,
                                      QWidget *secondaryViewContainer = nullptr);

    bool isInToolbar() const;

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

protected:
    /**
     * There should always ever be one navigatorsWidget for this action so
     * this method always returns the same widget and reparents it.
     * You normally don't have to use this method directly because
     * QWidgetAction::requestWidget() is used to obtain the navigatorsWidget
     * and to steal it from whereever it was prior.
     * @param parent the new parent of the navigatorsWidget.
     */
    QWidget *createWidget(QWidget *parent) override;

    /** @see QWidgetAction::deleteWidget() */
    void deleteWidget(QWidget *widget) override;

private:
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
     *                     will the button be visible.
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

    /**
     * Extracts the geometry information needed by adjustSpacing() from
     * ViewContainers. They are also monitored for size changes which
     * will lead to adjustSpacing() calls.
     */
    class ViewGeometriesHelper : public QObject
    {
    public:
        /**
         * @param navigatorsWidget       The QWidget of the navigatorsWidgetAction.
         * @param navigatorsWidgetAction is only used to call adjustSpacing() whenever that is
         *                               deemed necessary.
         */
        ViewGeometriesHelper(QWidget *navigatorsWidget, DolphinNavigatorsWidgetAction *navigatorsWidgetAction);

        /**
         * Calls m_navigatorsWidgetAction::adjustSpacing() when a watched object is resized.
         */
        bool eventFilter(QObject *watched, QEvent *event) override;

        /**
         * Sets the ViewContainers whose geometry is obtained when viewGeometries() is called.
         */
        void setViewContainers(QWidget *primaryViewContainer,
                               QWidget *secondaryViewContainer = nullptr);

        struct Geometries {
            int globalXOfNavigatorsWidget;
            int globalXOfPrimary;
            int widthOfPrimary;
            int globalXOfSecondary;
            int widthOfSecondary;
        };
        /**
         * @return a Geometries struct that contains values adjustSpacing() requires.
         */
        Geometries viewGeometries();

    private:
        QWidget *m_navigatorsWidget;
        /** Is only used to call adjustSpacing() whenever that is deemed necessary. */
        DolphinNavigatorsWidgetAction *m_navigatorsWidgetAction;

        QPointer<QWidget> m_primaryViewContainer;
        QPointer<QWidget> m_secondaryViewContainer;
    };

    ViewGeometriesHelper m_viewGeometriesHelper;
};

#endif // DOLPHINNAVIGATORSWIDGETACTION_H
