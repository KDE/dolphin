/***************************************************************************
 *   Copyright (C) 2006-2009 by Peter Penz <peter.penz19@gmail.com>        *
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

#ifndef INFORMATIONPANEL_H
#define INFORMATIONPANEL_H

#include <panels/panel.h>

class InformationPanelContent;
namespace KIO
{
    class Job;
}

/**
 * @brief Panel for showing meta information of one ore more selected items.
 */
class InformationPanel : public Panel
{
    Q_OBJECT

public:
    explicit InformationPanel(QWidget* parent = 0);
    virtual ~InformationPanel();

signals:
    void urlActivated(const KUrl& url);

public slots:
    /**
     * This is invoked to inform the panel that the user has selected a new
     * set of items.
     */
    void setSelection(const KFileItemList& selection);

    /**
     * Does a delayed request of information for the item \a item.
     * If within this delay InformationPanel::setUrl() or InformationPanel::setSelection()
     * are invoked, then the request will be skipped. Requesting a delayed item information
     * makes sense when hovering items.
     */
    void requestDelayedItemInfo(const KFileItem& item);

protected:
    /** @see Panel::urlChanged() */
    virtual bool urlChanged();

    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

    /** @see QWidget::resizeEvent() */
    virtual void resizeEvent(QResizeEvent* event);

    /** @see QWidget::contextMenuEvent() */
    virtual void contextMenuEvent(QContextMenuEvent* event);

private slots:
    /**
     * Shows the information for the item of the URL which has been provided by
     * InformationPanel::requestItemInfo() and provides default actions.
     */
    void showItemInfo();

    /**
     * Shows the information for the currently displayed folder as a result from
     * a stat job issued in showItemInfo().
     */
    void slotFolderStatFinished(KJob* job);

    /**
     * Triggered if the request for item information has timed out.
     * @see InformationPanel::requestDelayedItemInfo()
     */
    void slotInfoTimeout();

    /**
     * Resets the information panel to show the current
     * URL (InformationPanel::url()). Is called by
     * DolphinInformationPanel::markUrlAsInvalid().
     */
    void reset();

    void slotFileRenamed(const QString& source, const QString& dest);
    void slotFilesAdded(const QString& directory);
    void slotFilesChanged(const QStringList& files);
    void slotFilesRemoved(const QStringList& files);
    void slotEnteredDirectory(const QString& directory);
    void slotLeftDirectory(const QString& directory);

private:
    /** Assures that any pending item information request is cancelled. */
    void cancelRequest();

    /**
     * Shows the meta information for the current shown item inside
     * a label.
     */
    void showMetaInfo();

    /**
     * Returns true, if \a url is equal to the shown URL m_shownUrl.
     */
    bool isEqualToShownUrl(const KUrl& url) const;

    /**
     * Marks the URL as invalid and will reset the Information Panel
     * after a short delay. The reset is not done synchronously to
     * prevent expensive updates during temporary invalid URLs by
     * e. g. changing the directory.
     */
    void markUrlAsInvalid();

    void init();

private:
    bool m_initialized;
    bool m_pendingPreview;
    QTimer* m_infoTimer;
    QTimer* m_urlChangedTimer;
    QTimer* m_resetUrlTimer;

    // URL that is currently shown in the Information Panel.
    KUrl m_shownUrl;

    // URL candidate that will replace m_shownURL after a delay.
    // Used to remember URLs when hovering items.
    KUrl m_urlCandidate;

    // URL candidate that is marked as invalid (e. g. because the directory
    // has been deleted or the shown item has been renamed). The Information
    // Panel will be reset asynchronously to prevent unnecessary resets when
    // a directory has been changed.
    KUrl m_invalidUrlCandidate;

    KFileItem m_fileItem; // file item for m_shownUrl if available (otherwise null)
    KFileItemList m_selection;

    KIO::Job* m_folderStatJob;

    InformationPanelContent* m_content;
};

#endif // INFORMATIONPANEL_H
