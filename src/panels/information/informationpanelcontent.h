/***************************************************************************
 *   Copyright (C) 2009-2010 by Peter Penz <peter.penz19@gmail.com>        *
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

#ifndef INFORMATIONPANELCONTENT_H
#define INFORMATIONPANELCONTENT_H

#include <KConfig>
#include <KFileItem>
#include <KUrl>
#include <KVBox>

class KFileItemList;
class PhononWidget;
class PixmapViewer;
class PlacesItemModel;
class QPixmap;
class QString;
class QLabel;
class QScrollArea;

namespace Nepomuk2 {
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
    explicit InformationPanelContent(QWidget* parent = 0);
    virtual ~InformationPanelContent();

    /**
     * Shows the meta information for the item \p item.
     * The preview of the item is generated asynchronously,
     * the other meta information are fetched synchronously.
     */
    void showItem(const KFileItem& item);

    /**
     * Shows the meta information for the items \p items.
     */
    void showItems(const KFileItemList& items);

    /**
     * Opens a menu which allows to configure which meta information
     * should be shown.
     *
     * TODO: Move this code to the class InformationPanel
     */
    void configureSettings(const QList<QAction*>& customContextMenuActions);

signals:
    void urlActivated( const KUrl& url );

protected:
    /** @see QObject::eventFilter() */
    virtual bool eventFilter(QObject* obj, QEvent* event);

private slots:
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

    /**
     * Is invoked after the file meta data configuration dialog has been
     * closed and refreshes the visibility of the meta data.
     */
    void refreshMetaData();

private:
    /**
     * Checks whether the an URL is repesented by a place. If yes,
     * then the place icon and name are shown instead of a preview.
     * @return True, if the URL represents exactly a place.
     * @param url The url to check.
     */
    bool applyPlace(const KUrl& url);

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

private:
    KFileItem m_item;

    bool m_pendingPreview;
    QTimer* m_outdatedPreviewTimer;

    PixmapViewer* m_preview;
    PhononWidget* m_phononWidget;
    QLabel* m_nameLabel;
    Nepomuk2::FileMetaDataWidget* m_metaDataWidget;
    QScrollArea* m_metaDataArea;

    PlacesItemModel* m_placesItemModel;
};

#endif // INFORMATIONPANELCONTENT_H
