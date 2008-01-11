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

#include <kurl.h>

#include <QList>
#include <QObject>
#include <QPixmap>

class DolphinModel;
class KFileItem;
class KFileItemList;
class KJob;

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
    IconManager(QObject* parent, DolphinModel* model);
    virtual ~IconManager();
    void setShowPreview(bool show);
    bool showPreview() const;

private slots:
    /**
     * Generates a preview image for each file item in \a items.
     * The current preview settings (maximum size, 'Show Preview' menu)
     * are respected.
     */
    void generatePreviews(const KFileItemList& items);

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
    /**
     * Returns true, if the item \a item has been cut into
     * the clipboard.
     */
    bool isCutItem(const KFileItem& item) const;

    /** Applies an item effect to all cut items. */
    void applyCutItemEffect();

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
   QList<KJob*> m_previewJobs;
   DolphinModel* m_dolphinModel;

   QList<CutItem> m_cutItemsCache;
};

inline bool IconManager::showPreview() const
{
    return m_showPreview;
}

#endif
