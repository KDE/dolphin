/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef ICONMANAGER_H
#define ICONMANAGER_H

#include <kfileitem.h>
#include <kurl.h>

#include <QList>
#include <QObject>
#include <QPixmap>

class DolphinModel;
class DolphinSortFilterProxyModel;
class KJob;
class QAbstractItemView;

/**
 * @brief Manages the icon state of a directory model.
 *
 * Per default a preview is generated for each item.
 * Additionally the clipboard is checked for cut items.
 * The icon state for cut items gets dimmed automatically.
 */
class IconManager : public QObject
{
    Q_OBJECT

public:
    IconManager(QAbstractItemView* parent, DolphinSortFilterProxyModel* model);
    virtual ~IconManager();
    void setShowPreview(bool show);
    bool showPreview() const;
    void updatePreviews();

private slots:
    /**
     * Updates the icons of for each item in \a items by making hidden
     * items semitransparent and generating previews. The current preview
     * settings (maximum size, 'Show Preview' menu) are respected.
     */
    void updateIcons(const KFileItemList& items);

    /**
     * Replaces the icon of the item \a item by the preview pixmap
     * \a pixmap.
     */
    void replaceIcon(const KFileItem& item, const QPixmap& pixmap);

    /**
     * Is invoked when the preview job has been finished and
     * set m_previewJob to 0.
     */
    void slotPreviewJobFinished(KJob* job);

    /** Synchronizes the item icon with the clipboard of cut items. */
    void updateCutItems();

private:
    void generatePreviews(const KFileItemList &items);

    /**
     * Returns true, if the item \a item has been cut into
     * the clipboard.
     */
    bool isCutItem(const KFileItem& item) const;

    /** Applies an item effect to all cut items. */
    void applyCutItemEffect();

    /** Applies an item effect to the hidden item \a hiddenItem. */
    void applyHiddenItemEffect(const KFileItem& hiddenItem);

    /**
     * Applies a frame around the icon. False is returned if
     * no frame has been added because the icon is too small.
     */
    bool applyImageFrame(QPixmap& icon);

    /**
     * Resizes the icon to \a maxSize if the icon size does not
     * fit into the maximum size. The aspect ratio of the icon
     * is kept.
     */
    void limitToSize(QPixmap& icon, const QSize& maxSize);

    /** Kills all ongoing preview jobs. */
    void killJobs();

private:
    /**
     * Remembers the original pixmap for an item before
     * the cut effect is applied.
     */
    struct CutItem
    {
        KUrl url;
        QPixmap pixmap;
    };

    bool m_showPreview;

    QAbstractItemView* m_view;
    QList<KJob*> m_previewJobs;
    DolphinModel* m_dolphinModel;
    DolphinSortFilterProxyModel* m_proxyModel;

    QList<CutItem> m_cutItemsCache;
};

inline bool IconManager::showPreview() const
{
    return m_showPreview;
}

#endif
