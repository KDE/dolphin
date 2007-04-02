/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (<peter.penz@gmx.at>)                *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
 *   Copyright (C) 2006 by Patrice Tremblay                                *
 *   Copyright (C) 2007 by Kevin Ottens (ervin@kde.org)                    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef KURLNAVIGATOR_H
#define KURLNAVIGATOR_H

#include <kurl.h>
#include <QWidget>

class KFilePlacesModel;
class QMouseEvent;

/**
 * @brief Navigation bar which contains the current shown URL.
 *
 * The URL navigator offers two modes:
 * - Editable:     Represents the 'classic' mode, where the current URL
 *                 is editable inside a line editor.
 * - Non editable: The URL is represented by a number of buttons, where
 *                 clicking on a button results in activating the URL
 *                 the button represents. This mode also supports drag
 *                 and drop of items.
 *
 * The mode can be changed by a toggle button located on the left side of
 * the navigator.
 *
 * The URL navigator also remembers the URL history and allows to go
 * back and forward within this history.
 */
class KUrlNavigator : public QWidget
{
    Q_OBJECT

public:
    KUrlNavigator(KFilePlacesModel* placesModel, const KUrl& url, QWidget* parent);
    virtual ~KUrlNavigator();

    /** Returns the current active URL. */
    const KUrl& url() const;

    /** Returns the portion of the current active URL up to the button at index. */
    KUrl url(int index) const;

    /** Returns the amount of items in the history */
    int historySize() const;

    /** Returns the saved position from the history */
    QPoint savedPosition() const;

    /**
     * Goes back one step in the URL history. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goBack();

    /**
     * Goes forward one step in the URL history. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goForward();

    /**
     * Goes up one step of the URL path. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goUp();

    /**
     * Goes to the home URL. The signals UrlNavigator::urlChanged
     * and UrlNavigator::historyChanged are submitted.
     */
    void goHome();

    /**
     * Sets the home URL used by goHome().
     */
    void setHomeUrl(const QString& homeUrl);

    /**
     * @return True, if the URL is editable by the user within a line editor.
     *         If false is returned, each part of the URL is presented by a button
     *         for fast navigation.
     */
    bool isUrlEditable() const;

    /**
     * Allows to edit the URL of the navigation bar if \a editable
     * is true, and sets the focus accordingly.
     * If \a editable is false, each part of
     * the URL is presented by a button for a fast navigation.
     */
    void setUrlEditable(bool editable);

    /**
     * Set the URL navigator to the active mode, if \a active
     * is true. The active mode is default. Using the URL navigator
     * in the inactive mode is useful when having split views,
     * where the inactive view is indicated by an inactive URL
     * navigator visually.
     */
    void setActive(bool active);

    /**
     * Returns true, if the URL navigator is in the active mode.
     * @see UrlNavigator::setActive()
     */
    bool isActive() const;

    /**
     * Sets whether or not to show hidden files in lists
     */
    void setShowHiddenFiles( bool show );

    /**
     * Returns true if the URL navigator is set to show hidden files
     */
    bool showHiddenFiles() const; // TODO rename, looks like a verb

    /**
     * Handles the dropping of the URLs \a urls to the given
     * destination \a destination and emits the signal urlsDropped.
     */
    void dropUrls(const KUrl::List& urls,
                  const KUrl& destination);

public Q_SLOTS:
    /**
     * Sets the current active URL.
     * The signals UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void setUrl(const KUrl& url);

    /**
     * Activates the URL navigator (UrlNavigator::isActive() will return true)
     * and emits the signal 'activationChanged()'.
     */
    void requestActivation();

    /**
     * Stores the coordinates of the contents into
     * the current history element.
     */
    void storeContentsPosition(int x, int y);

Q_SIGNALS:
    /**
     * Is emitted, if the URL navigator has been activated by
     * a user interaction.
     */
    void activated();

    /**
     * Is emitted, if the URL has been changed e. g. by
     * the user.
     * @see setUrl()
     */
    void urlChanged(const KUrl& url);

    /**
     * Is emitted, if the history has been changed. Usually
     * the history is changed if a new URL has been selected.
     */
    void historyChanged();

    /**
     * Is emitted if the URLs \a urls have been dropped
     * to the destination \a destination.
     */
    void urlsDropped(const KUrl::List& urls,
                     const KUrl& destination);

protected:
    /**
     * If the Escape key is pressed, the navigation bar should switch
     * to the browse mode.
     */
    virtual void keyReleaseEvent(QKeyEvent* event);

    /**
     * Paste the clipboard content as URL, if the middle mouse
     * button has been clicked.
     */
    virtual void mouseReleaseEvent(QMouseEvent* event);

private:
    Q_PRIVATE_SLOT(d, void slotReturnPressed(const QString& text))
    Q_PRIVATE_SLOT(d, void slotRemoteHostActivated())
    Q_PRIVATE_SLOT(d, void slotProtocolChanged(const QString& protocol))
    Q_PRIVATE_SLOT(d, void switchView())
    //Q_PRIVATE_SLOT(d, void slotRedirection(const KUrl&, const KUrl&))

private:
    class Private;
    Private* const d;

    Q_DISABLE_COPY( KUrlNavigator )
};

#endif
