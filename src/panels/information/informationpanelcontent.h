/*
 * SPDX-FileCopyrightText: 2009-2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef INFORMATIONPANELCONTENT_H
#define INFORMATIONPANELCONTENT_H

#include <KFileItem>
#include <config-baloo.h>

#include <QPointer>
#include <QUrl>
#include <QWidget>

class KFileItemList;
class PhononWidget;
class PixmapViewer;
class PlacesItemModel;
class QPixmap;
class QDialogButtonBox;
class QString;
class QLabel;
class QScrollArea;
class QGestureEvent;

namespace KIO {
  class PreviewJob;
}

namespace Baloo {
    class FileMetaDataWidget;
}

/**
 * @brief Manages the widgets that display the meta information
*         for file items of the Information Panel.
 */
class InformationPanelContent : public QWidget
{
    Q_OBJECT

public:
    explicit InformationPanelContent(QWidget* parent = nullptr);
    ~InformationPanelContent() override;

    /**
     * Shows the meta information for the item \p item.
     * The preview of the item is generated asynchronously,
     * the other meta information are fetched synchronously.
     */
    void showItem(const KFileItem& item);

    /**
     * Shows the meta information for the items \p items and its preview
     */
    void showItems(const KFileItemList& items);

    KFileItemList items();

    /**
     * Refreshes the preview display, hiding it if needed
     */
    void refreshPreview();

    /**
     * Switch the metadatawidget into configuration mode
     */
    void configureShownProperties();

    /*
     * Set the auto play media mode for the file previewed
     * Eventually starting media playback when turning it on
     * But not stopping it when turning it off
     */
    void setPreviewAutoPlay(bool autoPlay);

Q_SIGNALS:
    void urlActivated( const QUrl& url );
    void configurationFinished();
    void contextMenuRequested(const QPoint& pos);

public Q_SLOTS:
    /**
     * Is invoked after the file meta data configuration dialog has been
     * closed and refreshes the displayed meta data by the panel.
     */
    void refreshMetaData();

protected:
    /** @see QObject::eventFilter() */
    bool eventFilter(QObject* obj, QEvent* event) override;

    bool event(QEvent * event) override;

private Q_SLOTS:
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

    /**
     * Marks the currently shown preview as outdated
     * by greying the content.
     */
    void markOutdatedPreview();

    void slotHasVideoChanged(bool hasVideo);

private:
    /**
     * Sets the text for the label \a m_nameLabel and assures that the
     * text is split in a way that it can be wrapped within the
     * label width (QLabel::setWordWrap() does not work if the
     * text represents one extremely long word).
     */
    void setNameLabelText(const QString& text);

    /**
     * Adjusts the sizes of the widgets dependent on the available
     * width given by \p width.
     */
    void adjustWidgetSizes(int width);

    /**
     * Refreshes the image in the PixmapViewer
     */
    void refreshPixmapView();

    bool gestureEvent(QGestureEvent* event);

private:
    KFileItem m_item;

    QPointer<KIO::PreviewJob> m_previewJob;
    QTimer* m_outdatedPreviewTimer;

    PixmapViewer* m_preview;
    PhononWidget* m_phononWidget;
    QLabel* m_nameLabel;
    Baloo::FileMetaDataWidget* m_metaDataWidget;
    QScrollArea* m_metaDataArea;
    QLabel* m_configureLabel;
    QDialogButtonBox* m_configureButtons;

    PlacesItemModel* m_placesItemModel;
    bool m_isVideo;
};

#endif // INFORMATIONPANELCONTENT_H
