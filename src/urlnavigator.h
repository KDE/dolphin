/**************************************************************************
*   Copyright (C) 2006 by Peter Penz                                      *
*   peter.penz@gmx.at                                                     *
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

#ifndef UrlNAVIGATOR_H
#define UrlNAVIGATOR_H


//Added by qt3to4:
#include <QLabel>
#include <Q3ValueList>
#include <QKeyEvent>
#include <Q3PopupMenu>
#include <kurl.h>
#include <qstring.h>
#include <kvbox.h>

class QComboBox;
class QLabel;
class QLineEdit;
class Q3PopupMenu;
class QCheckBox;

class KUrl;
class KFileItem;
class KUrlComboBox;

class BookmarkSelector;
class DolphinView;
class ProtocolCombo;

/**
 * @brief Navigation bar which contains the current shown Url.
 *
 * The Url navigator offers two modes:
 * - Editable:     Represents the 'classic' mode, where the current Url
 *                 is editable inside a line editor.
 * - Non editable: The Url is represented by a number of buttons, where
 *                 clicking on a button results in activating the Url
 *                 the button represents. This mode also supports drag
 *                 and drop of items.
 *
 * The mode can be changed by a toggle button located on the left side of
 * the navigator.
 *
 * The Url navigator also remembers the Url history and allows to go
 * back and forward within this history.
 *
 * @author Peter Penz
*/

typedef Q3ValueList<KUrl> UrlStack;

class UrlNavigator : public KHBox
{
    Q_OBJECT

public:
    /**
     * @brief Represents the history element of an Url.
     *
     * A history element contains the Url, the name of the current file
     * (the 'current file' is the file where the cursor is located) and
     * the x- and y-position of the content.
     */
    class HistoryElem {
    public:
        HistoryElem();
        HistoryElem(const KUrl& url);
        ~HistoryElem(); // non virtual

        const KUrl& url() const { return m_url; }

        void setCurrentFileName(const QString& name) { m_currentFileName = name; }
        const QString& currentFileName() const { return m_currentFileName; }

        void setContentsX(int x) { m_contentsX = x; }
        int contentsX() const { return m_contentsX; }

        void setContentsY(int y) { m_contentsY = y; }
        int contentsY() const { return m_contentsY; }

    private:
        KUrl m_url;
        QString m_currentFileName;
        int m_contentsX;
        int m_contentsY;
    };

    UrlNavigator(const KUrl& url, DolphinView* dolphinView);
    virtual ~UrlNavigator();

    /**
     * Sets the current active Url.
     * The signals UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void setUrl(const KUrl& url);

    /** Returns the current active Url. */
    const KUrl& url() const;

    /** Returns the portion of the current active Url up to the button at index. */
    KUrl url(int index) const;

    /**
     * Returns the complete Url history. The index 0 indicates the oldest
     * history element.
     * @param index     Output parameter which indicates the current
     *                  index of the location.
     */
    const Q3ValueList<HistoryElem>& history(int& index) const;

    /**
     * Goes back one step in the Url history. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goBack();

    /**
     * Goes forward one step in the Url history. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goForward();

    /**
     * Goes up one step of the Url path. The signals
     * UrlNavigator::urlChanged and UrlNavigator::historyChanged
     * are submitted.
     */
    void goUp();

    /**
     * Goes to the home Url. The signals UrlNavigator::urlChanged
     * and UrlNavigator::historyChanged are submitted.
     */
    void goHome();

    /**
     * @return True, if the Url is editable by the user within a line editor.
     *         If false is returned, each part of the Url is presented by a button
     *         for fast navigation.
     */
    bool isUrlEditable() const;

    /**
     * Switches to the edit mode and assures that the keyboard focus
     * is assigned.
     */
    void editUrl(bool editOrBrowse); //TODO: switch to an enum

    DolphinView* dolphinView() const;

signals:
    void urlChanged(const KUrl& url);
    void historyChanged();

protected:
    /** If the Escape key is pressed, the navigation bar should switch
        to the browse mode. */
    virtual void keyReleaseEvent(QKeyEvent* event);

private slots:
    void slotReturnPressed(const QString& text);
    void slotUrlActivated(const KUrl& url);
    void slotRemoteHostActivated();
    void slotProtocolChanged(const QString& protocol);

    void slotRequestActivation();
    void slotBookmarkActivated(int index);

    void slotRedirection(const KUrl&, const KUrl&);

    /**
     * Stores the coordinates of the moved content into
     * the current history element. Is usually triggered
     * by the signal 'contentsMoved' emitted by DolphinView.
     */
    void slotContentsMoved(int x, int y);

    /**
     * Switches the navigation bar between the editable and noneditable
     * state (see setUrlEditable()) and is connected to the clicked signal
     * of the navigation bar button.
     */
    void slotClicked();

private:
    int m_historyIndex;
    DolphinView* m_dolphinView;
    Q3ValueList<HistoryElem> m_history;
    QCheckBox* m_toggleButton;
    BookmarkSelector* m_bookmarkSelector;
    KUrlComboBox* m_pathBox;
    ProtocolCombo* m_protocols;
    QLabel* m_protocolSeparator;
    QLineEdit* m_host;
    Q3ValueList<QWidget*> m_navButtons;
    //UrlStack m_urls;

    /**
     * Allows to edit the Url of the navigation bar if \a editable
     * is true. If \a editable is false, each part of
     * the Url is presented by a button for a fast navigation.
     */
    void setUrlEditable(bool editable);

    /**
     * Updates the history element with the current file item
     * and the contents position.
     */
    void updateHistoryElem();
    void updateContent();
};

#endif
