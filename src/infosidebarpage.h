/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef INFOSIDEBARPAGE_H
#define INFOSIDEBARPAGE_H

#include <sidebarpage.h>

#include <qpushbutton.h>
#include <QPixmap>
#include <QEvent>
#include <QLabel>
#include <QList>

#include <kurl.h>
#include <kmimetype.h>
#include <kdesktopfileactions.h>
#include <kvbox.h>

namespace KIO {
    class Job;
}

class QPixmap;
class QIcon;
class QString;
class QPainter;
class KFileItem;
class QLabel;
class KVBox;
class PixmapViewer;
class MetaDataWidget;

/**
 * @brief Prototype for a information sidebar.
 *
 * Will be exchanged in future releases by pluggable sidebar pages...
 */
class InfoSidebarPage : public SidebarPage
{
	Q_OBJECT

public:
    explicit InfoSidebarPage(QWidget* parent = 0);
    virtual ~InfoSidebarPage();

public slots:
    void setUrl(const KUrl& url);
    void setSelection(const KFileItemList& selection);

private slots:
    /**
     * Does a delayed request of information for the item of the given Url and
     * provides default actions.
     *
     * @see InfoSidebarPage::showItemInfo()
     */
    void requestDelayedItemInfo(const KUrl& url);

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
     * Checks whether the an Url is repesented by a bookmark. If yes,
     * then the bookmark icon and name are shown instead of a preview.
     * @return True, if the Url represents exactly a bookmark.
     * @param url The url to check.
     */
    bool applyBookmark(const KUrl& url);

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
	KFileItemList m_currentSelection;

    PixmapViewer* m_preview;
    QLabel* m_name;

    QString m_infoLines;
    QLabel* m_infos;

    KVBox* m_actionBox;
    QVector<KDesktopFileActions::Service> m_actionsVector;

    MetaDataWidget* m_metadataWidget;
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
    virtual void paintEvent(QPaintEvent* event);
    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);

private slots:
    void slotReleased();

private:
    bool m_hover;
    int m_index;
};

#endif // INFOSIDEBARPAGE_H
