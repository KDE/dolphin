/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _INFOSIDEBARPAGE_H_
#define _INFOSIDEBARPAGE_H_

#include <sidebarpage.h>

#include <q3valuevector.h>
#include <qpushbutton.h>
//Added by qt3to4:
#include <QPixmap>
#include <QEvent>
#include <QLabel>
#include <Q3PtrList>

#include <kurl.h>
#include <kmimetype.h>
#include <kdedesktopmimetype.h>

namespace KIO {
    class Job;
};

class QPixmap;
class QIcon;
class QString;
class QPainter;
class KFileItem;
class QLabel;
class Q3VBox;
class Q3Grid;
class PixmapViewer;

/**
 * @brief Prototype for a information sidebar.
 *
 * Will be exchanged in future releases by pluggable sidebar pages...
 */
class InfoSidebarPage : public SidebarPage
{
	Q_OBJECT

public:
    InfoSidebarPage(QWidget* parent);
    virtual ~InfoSidebarPage();

protected:
    /** @see SidebarPage::activeViewChanged() */
    virtual void activeViewChanged();

private slots:
    /**
     * Does a delayed request of information for the item of the given Url and
     * provides default actions.
     *
     * @see InfoSidebarPage::showItemInfo()
     */
    void requestDelayedItemInfo(const KUrl& url);

    /**
     * Does a request of information for the item of the given Url and
     * provides default actions.
     *
     * @see InfoSidebarPage::showItemInfo()
     */
    void requestItemInfo(const KUrl& url);

    /**
     * Shows the information for the item of the Url which has been provided by
     * InfoSidebarPage::requestItemInfo() and provides default actions.
     */
    void showItemInfo();

    /**
     * Triggered if the request for item information has timed out.
     * @see InfoSidebarPage::requestDelayedItemInfo()
     */
    void slotTimeout();

    /**
     * Is invoked if no preview is available for the item. In this
     * case the icon will be shown.
     */
    void slotPreviewFailed(const KFileItem* item);

    /**
     * Is invoked if a preview is available for the item. The preview
     * \a pixmap is shown inside the info page.
     */
    void gotPreview(const KFileItem* item, const QPixmap& pixmap);

    /**
     * Starts the service of m_actionsVector with the index \index on
     * the shown Url (or the selected items if available).
     */
    void startService(int index);

private:
    /**
     * Connects to signals from the currently active Dolphin view to get
     * informed about highlighting changes.
     */
    void connectToActiveView();

    /**
     * Checks whether the current Url is repesented by a bookmark. If yes,
     * then the bookmark icon and name are shown instead of a preview.
     * @return True, if the Url represents exactly a bookmark.
     */
    bool applyBookmark();

    /** Assures that any pending item information request is cancelled. */
    void cancelRequest();

    // TODO: the following methods are just a prototypes for meta
    // info generation...
    void createMetaInfo();
    void addInfoLine(const QString& labelText,
                     const QString& infoText);
    void beginInfoLines();
    void endInfoLines();

    /**
     * Returns true, if the string \a key represents a meta information
     * that should be shown.
     */
    bool showMetaInfo(const QString& key) const;

    /**
     * Inserts the available actions to the info page for the given item.
     */
    void insertActions();

    bool m_multipleSelection;
    bool m_pendingPreview;
    QTimer* m_timer;
    KUrl m_shownUrl;
    KUrl m_urlCandidate;

    PixmapViewer* m_preview;
    QLabel* m_name;

    int m_currInfoLineIdx;
    Q3Grid* m_infoGrid;
    Q3PtrList<QLabel> m_infoWidgets;       // TODO: use children() from QObject instead

    Q3VBox* m_actionBox;
    Q3PtrList<QWidget> m_actionWidgets;    // TODO: use children() from QObject instead
    Q3ValueVector<KDEDesktopMimeType::Service> m_actionsVector;
};

// TODO #1: move to SidebarPage?
// TODO #2: quite same button from the optical point of view as UrlNavigatorButton
// -> provide helper class or common base class
class ServiceButton : public QPushButton
{
    Q_OBJECT

public:
    ServiceButton(const QIcon& icon,
                  const QString& text,
                  QWidget* parent,
                  int index);
    virtual ~ServiceButton();

signals:
    void requestServiceStart(int index);

protected:
    virtual void drawButton(QPainter* painter);
    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);

private slots:
    void slotReleased();

private:
    bool m_hover;
    int m_index;
};

#endif // _INFOSIDEBARPAGE_H_
