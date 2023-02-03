/*
 * SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEWMODECONTROLLER_H
#define VIEWMODECONTROLLER_H

#include "dolphin_export.h"
#include "views/dolphinview.h"

#include <QObject>
#include <QUrl>

/**
 * @brief Allows the DolphinView to control the view implementations for the
 *        different view modes.
 *
 * The view implementations (DolphinIconsView, DolphinDetailsView, DolphinColumnView)
 * connect to signals of the ViewModeController to react on changes. The view
 * implementations get only read-access to the ViewModeController.
 */
class DOLPHIN_EXPORT ViewModeController : public QObject
{
    Q_OBJECT

public:
    explicit ViewModeController(QObject *parent = nullptr);
    ~ViewModeController() override;

    /**
     * @return URL that is shown by the view mode implementation.
     */
    QUrl url() const;

    /**
     * Sets the URL to \a url and does nothing else. Called when
     * a redirection happens. See ViewModeController::setUrl()
     */
    void redirectToUrl(const QUrl &url);

    /**
     * Informs the view mode implementation about a change of the activation
     * state by emitting the signal activationChanged().
     */
    void indicateActivationChange(bool active);

    /**
     * Sets the zoom level to \a level and emits the signal zoomLevelChanged().
     * It must be assured that the used level is inside the range
     * ViewModeController::zoomLevelMinimum() and
     * ViewModeController::zoomLevelMaximum().
     */
    void setZoomLevel(int level);
    int zoomLevel() const;

    /**
     * Sets the name filter to \a and emits the signal nameFilterChanged().
     */
    void setNameFilter(const QString &nameFilter);
    QString nameFilter() const;

public Q_SLOTS:
    /**
     * Sets the URL to \a url and emits the signals cancelPreviews() and
     * urlChanged() if \a url is different for the current URL.
     */
    void setUrl(const QUrl &url);

Q_SIGNALS:
    /**
     * Is emitted if the URL has been changed by ViewModeController::setUrl().
     */
    void urlChanged(const QUrl &url);

    /**
     * Is emitted, if ViewModeController::indicateActivationChange() has been
     * invoked. The view mode implementation may update its visual state
     * to represent the activation state.
     */
    void activationChanged(bool active);

    /**
     * Is emitted if the name filter has been changed by
     * ViewModeController::setNameFilter().
     */
    void nameFilterChanged(const QString &nameFilter);

    /**
     * Is emitted if the zoom level has been changed by
     * ViewModeController::setZoomLevel().
     */
    void zoomLevelChanged(int level);

    /**
     * Is emitted if pending previews should be canceled (e. g. because of an URL change).
     */
    void cancelPreviews();

private:
    int m_zoomLevel;
    QString m_nameFilter;
    QUrl m_url;
};

#endif
