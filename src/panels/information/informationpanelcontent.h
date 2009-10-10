/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#include <panels/panel.h>

#include <kconfig.h>
#include <kurl.h>
#include <kvbox.h>

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
 * @brief Manages the widgets that display the meta information
*         for file items of the Information Panel.
 */
class InformationPanelContent : public Panel
{
    Q_OBJECT

public:
    explicit InformationPanelContent(QWidget* parent = 0);
    virtual ~InformationPanelContent();

    /**
     * Shows the meta information for the item \p item.
     * The preview of the item is generated asynchronously,
     * the other meta informations are fetched synchronously.
     */
    void showItem(const KFileItem& item);

    /**
     * Shows the meta information for the items \p items.
     */
    void showItems(const KFileItemList& items);

    /**
     * Opens a menu which allows to configure which meta information
     * should be shown.
     */
    void configureSettings();

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

    /**
     * Sets the text for the label \a m_nameLabel and assures that the
     * text is split in a way that it can be wrapped within the
     * label width (QLabel::setWordWrap() does not work if the
     * text represents one extremely long word).
     */
    void setNameLabelText(const QString& text);

    /**
     * Assures that the settings for the meta information
     * are initialized with proper default values.
     */
    void initMetaInfoSettings(KConfigGroup& group);

    /**
     * Temporary helper method for KDE 4.3 as we currently don't get
     * translated labels for Nepmok literals: Replaces camelcase labels
     * like "fileLocation" by "File Location:".
     */
    QString tunedLabel(const QString& label) const;

    void init();

private:
    KFileItem m_item;

    bool m_pendingPreview;
    QTimer* m_outdatedPreviewTimer;
    QLabel* m_nameLabel;

    PixmapViewer* m_preview;
    KSeparator* m_previewSeparator;
    PhononWidget* m_phononWidget;
    MetaDataWidget* m_metaDataWidget;
    QScrollArea* m_metaDataArea;
};

#endif // INFORMATIONPANELCONTENT_H
