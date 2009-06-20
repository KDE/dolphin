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

#ifndef INFORMATIONPANEL_H
#define INFORMATIONPANEL_H

#include <panels/panel.h>

#include <QPushButton>
#include <QPixmap>
#include <QEvent>
#include <QLabel>
#include <QList>

#include <kconfig.h>
#include <kurl.h>
#include <kmimetype.h>
#include <kdesktopfileactions.h>
#include <kvbox.h>

class InformationPanelDialog;
class PhononWidget;
class PixmapViewer;
class MetaDataWidget;
class MetaTextLabel;
class QPixmap;
class QString;
class KFileItem;
class KSeparator;
class QLabel;
class QScrollArea;

/**
 * @brief Panel for showing meta information of one ore more selected items.
 */
class InformationPanel : public Panel
{
    Q_OBJECT

public:
    explicit InformationPanel(QWidget* parent = 0);
    virtual ~InformationPanel();

    /** @see QWidget::sizeHint() */
    virtual QSize sizeHint() const;

public slots:
    /** @see Panel::setUrl() */
    virtual void setUrl(const KUrl& url);

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
    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

    /** @see QWidget::resizeEvent() */
    virtual void resizeEvent(QResizeEvent* event);

    /** @see QObject::eventFilter() */
    virtual bool eventFilter(QObject* obj, QEvent* event);

    /** @see QWidget::contextMenuEvent() */
    virtual void contextMenuEvent(QContextMenuEvent* event);

private slots:
    /**
     * Shows the information for the item of the URL which has been provided by
     * InformationPanel::requestItemInfo() and provides default actions.
     */
    void showItemInfo();

    /**
     * Triggered if the request for item information has timed out.
     * @see InformationPanel::requestDelayedItemInfo()
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

    void slotPlayingStarted();
    void slotPlayingStopped();

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
     * Returns the item for file where the preview and meta information
     * should be received, if InformationPanel::showMultipleSelectionInfo()
     * returns false.
     */
    KFileItem fileItem() const;

    /**
     * Returns true, if the meta information should be shown for
     * the multiple selected items that are stored in
     * m_selection. If true is returned, it is assured that
     * m_selection.count() > 1. If false is returned, the meta
     * information should be shown for the file
     * InformationPanel::fileUrl();
     */
    bool showMultipleSelectionInfo() const;

    /**
     * Returns true, if \a url is equal to the shown URL m_shownUrl.
     */
    bool isEqualToShownUrl(const KUrl& url) const;

    /**
     * Sets the text for the label \a m_nameLabel and assures that the
     * text is split in a way that it can be wrapped within the
     * label width (QLabel::setWordWrap() does not work if the
     * text represents one extremely long word).
     */
    void setNameLabelText(const QString& text);

    /**
     * Resets the information panel to show the current
     * URL (InformationPanel::url()).
     */
    void reset();

    /**
     * Assures that the settings for the meta information
     * are initialized with proper default values.
     */
    void initMetaInfoSettings(KConfigGroup& group);

    void updatePhononWidget();

    /**
     * Temporary helper method for KDE 4.3 as we currently don't get
     * translated labels for Nepmok literals: Replaces camelcase labels
     * like "fileLocation" by "File Location:".
     */
    QString tunedLabel(const QString& label) const;

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
    KSeparator* m_previewSeparator;
    PhononWidget* m_phononWidget;
    MetaDataWidget* m_metaDataWidget;
    KSeparator* m_metaDataSeparator;

    QScrollArea* m_metaTextArea;
    MetaTextLabel* m_metaTextLabel;
};

#endif // INFORMATIONPANEL_H
