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

#include <QtGui/QPushButton>
#include <QtGui/QPixmap>
#include <QtCore/QEvent>
#include <QtGui/QLabel>
#include <QtCore/QList>

#include <kurl.h>
#include <kmimetype.h>
#include <kdesktopfileactions.h>
#include <kvbox.h>

class QPixmap;
class QString;
class KFileItem;
class QLabel;
class PixmapViewer;
class MetaDataWidget;
class MetaTextLabel;

/**
 * @brief Sidebar for showing meta information of one ore more selected items.
 */
class InfoSidebarPage : public SidebarPage
{
    Q_OBJECT

public:
    explicit InfoSidebarPage(QWidget* parent = 0);
    virtual ~InfoSidebarPage();

    /** @see QWidget::sizeHint() */
    virtual QSize sizeHint() const;

public slots:
    /** @see SidebarPage::setUrl() */
    virtual void setUrl(const KUrl& url);

    /**
     * This is invoked to inform the sidebar that the user has selected a new
     * set of items.
     */
    void setSelection(const KFileItemList& selection);

    /**
     * Does a delayed request of information for the item \a item.
     * If within this delay InfoSidebarPage::setUrl() or InfoSidebarPage::setSelection()
     * are invoked, then the request will be skipped. Requesting a delayed item information
     * makes sense when hovering items.
     */
    void requestDelayedItemInfo(const KFileItem& item);

protected:
    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

    /** @see QWidget::resizeEvent() */
    virtual void resizeEvent(QResizeEvent* event);

private slots:
    /**
     * Shows the information for the item of the URL which has been provided by
     * InfoSidebarPage::requestItemInfo() and provides default actions.
     */
    void showItemInfo();

    /**
     * Triggered if the request for item information has timed out.
     * @see InfoSidebarPage::requestDelayedItemInfo()
     */
    void slotInfoTimeout();

    /**
     * Marks the currently shown preview as outdated
     * by greying the content.
     */
    void markOutdatedPreview();

    /**
     * Is invoked if no preview is available for the item. In this
     * case the icon will be shown.
     */
    void showIcon(const KFileItem& item);

    /**
     * Is invoked if a preview is available for the item. The preview
     * \a pixmap is shown inside the info page.
     */
    void showPreview(const KFileItem& item, const QPixmap& pixmap);

    void slotFileRenamed(const QString& source, const QString& dest);
    void slotFilesAdded(const QString& directory);
    void slotFilesChanged(const QStringList& files);
    void slotFilesRemoved(const QStringList& files);
    void slotEnteredDirectory(const QString& directory);
    void slotLeftDirectory(const QString& directory);

private:
    /**
     * Checks whether the an URL is repesented by a place. If yes,
     * then the place icon and name are shown instead of a preview.
     * @return True, if the URL represents exactly a place.
     * @param url The url to check.
     */
    bool applyPlace(const KUrl& url);

    /** Assures that any pending item information request is cancelled. */
    void cancelRequest();

    /**
     * Shows the meta information for the current shown item inside
     * a label.
     */
    void showMetaInfo();

    /**
     * Converts the meta key \a key to a readable format into \a text.
     * Returns true, if the string \a key represents a meta information
     * that should be shown. If false is returned, \a text is not modified.
     */
    bool convertMetaInfo(const QString& key, QString& text) const;

    /**
     * Returns the URL of the file where the preview and meta information
     * should be received, if InfoSidebarPage::showMultipleSelectionInfo()
     * returns false.
     */
    KUrl fileUrl() const;

    /**
     * Returns true, if the meta information should be shown for
     * the multiple selected items that are stored in
     * m_selection. If true is returned, it is assured that
     * m_selection.count() > 1. If false is returned, the meta
     * information should be shown for the file
     * InfosidebarPage::fileUrl();
     */
    bool showMultipleSelectionInfo() const;

    void init();

private:
    bool m_initialized;
    bool m_pendingPreview;
    QTimer* m_infoTimer;
    QTimer* m_outdatedPreviewTimer;
    KUrl m_shownUrl;      // URL that is shown as info
    KUrl m_urlCandidate;  // URL candidate that will replace m_shownURL after a delay
    KFileItem m_fileItem; // file item for m_shownUrl if available (otherwise null)
    KFileItemList m_selection;

    QLabel* m_nameLabel;
    PixmapViewer* m_preview;
    MetaDataWidget* m_metaDataWidget;
    MetaTextLabel* m_metaTextLabel;
};

#endif // INFOSIDEBARPAGE_H
