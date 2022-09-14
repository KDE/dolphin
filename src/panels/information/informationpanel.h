/*
 * SPDX-FileCopyrightText: 2006-2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef INFORMATIONPANEL_H
#define INFORMATIONPANEL_H

#include "panels/panel.h"

#include <KFileItem>

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
    explicit InformationPanel(QWidget* parent = nullptr);
    ~InformationPanel() override;

Q_SIGNALS:
    void urlActivated(const QUrl& url);

public Q_SLOTS:
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

    void slotFilesItemChanged(const KFileItemList &changedFileItems);

protected:
    /** @see Panel::urlChanged() */
    bool urlChanged() override;

    /** @see QWidget::showEvent() */
    void showEvent(QShowEvent* event) override;

    /** @see QWidget::resizeEvent() */
    void resizeEvent(QResizeEvent* event) override;

    /** @see QWidget::contextMenuEvent() */
    void contextMenuEvent(QContextMenuEvent* event) override;

private Q_SLOTS:
    /**
     * Shows the information for the item of the URL which has been provided by
     * InformationPanel::requestDelayedItemInfo() and provides default actions.
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
     * Returns true, if \a url is equal to the shown URL m_shownUrl.
     */
    bool isEqualToShownUrl(const QUrl& url) const;

    /**
     * Marks the URL as invalid and will reset the Information Panel
     * after a short delay. The reset is not done synchronously to
     * prevent expensive updates during temporary invalid URLs by
     * e. g. changing the directory.
     */
    void markUrlAsInvalid();

    /**
     * Opens a menu which allows to configure which meta information
     * should be shown.
     */
    void showContextMenu(const QPoint &point);

    void init();

private:
    bool m_initialized;
    QTimer* m_infoTimer;
    QTimer* m_urlChangedTimer;
    QTimer* m_resetUrlTimer;

    // URL that is currently shown in the Information Panel.
    QUrl m_shownUrl;

    // URL candidate that will replace m_shownURL after a delay.
    // Used to remember URLs when hovering items.
    QUrl m_urlCandidate;

    // URL candidate that is marked as invalid (e. g. because the directory
    // has been deleted or the shown item has been renamed). The Information
    // Panel will be reset asynchronously to prevent unnecessary resets when
    // a directory has been changed.
    QUrl m_invalidUrlCandidate;

    KFileItem m_hoveredItem;
    KFileItemList m_selection;

    KIO::Job* m_folderStatJob;

    InformationPanelContent* m_content;
    bool m_inConfigurationMode = false;
};

#endif // INFORMATIONPANEL_H
