/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef VIEWMODECONTROLLER_H
#define VIEWMODECONTROLLER_H

#include <KUrl>
#include <QObject>
#include <libdolphin_export.h>
#include <views/dolphinview.h>

/**
 * @brief Allows the DolphinView to control the view implementations for the
 *        different view modes.
 *
 * The view implementations (DolphinIconsView, DolphinDetailsView, DolphinColumnView)
 * connect to signals of the ViewModeController to react on changes. The view
 * implementations get only read-access to the ViewModeController.
 */
class LIBDOLPHINPRIVATE_EXPORT ViewModeController : public QObject
{
    Q_OBJECT

public:
    explicit ViewModeController(QObject* parent = 0);
    virtual ~ViewModeController();

    /**
     * @return URL that is shown by the view mode implementation.
     */
    KUrl url() const;

    /**
     * Sets the URL to \a url and does nothing else. Called when
     * a redirection happens. See ViewModeController::setUrl()
     */
    void redirectToUrl(const KUrl& url);

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
    void setNameFilter(const QString& nameFilter);
    QString nameFilter() const;

    /**
     * Requests the view mode implementation to hide tooltips.
     */
    void requestToolTipHiding();

public slots:
    /**
     * Sets the URL to \a url and emits the signals cancelPreviews() and
     * urlChanged() if \a url is different for the current URL.
     */
    void setUrl(const KUrl& url);

signals:
    /**
     * Is emitted if the URL has been changed by ViewModeController::setUrl().
     */
    void urlChanged(const KUrl& url);

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
    void nameFilterChanged(const QString& nameFilter);

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
    KUrl m_url;
};

#endif
