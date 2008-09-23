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

#ifndef KFILEPREVIEWGENERATOR_H
#define KFILEPREVIEWGENERATOR_H

#include <QObject>

class AbstractViewAdapter;
class KDirModel;
class QAbstractItemView;
class QAbstractProxyModel;

/**
 * @brief Generates previews for files of an item view.
 *
 * Per default a preview is generated for each item.
 * Additionally the clipboard is checked for cut items.
 * The icon state for cut items gets dimmed automatically.
 *
 * The following strategy is used when creating previews:
 * - The previews for currently visible items are created before
 *   the previews for invisible items.
 * - If the user changes the visible area by using the scrollbars,
 *   all pending previews get paused. As soon as the user stays
 *   on the same position for a short delay, the previews are
 *   resumed. Also in this case the previews for the visible items
 *   are generated first.
 */
class KFilePreviewGenerator : public QObject
{
    Q_OBJECT

public:
    /**
     * @param parent  Item view containing the file items where previews should
     *                be generated. It is mandatory that the item view specifies
     *                an icon size by QAbstractItemView::setIconSize(), otherwise
     *                no previews will be generated.
     * @param model   Model of the item view.
     */
    KFilePreviewGenerator(QAbstractItemView* parent, QAbstractProxyModel* model);
    
    /** @internal */
    KFilePreviewGenerator(AbstractViewAdapter* parent, QAbstractProxyModel* model);
    
    virtual ~KFilePreviewGenerator();
    
    /**
     * If \a show is set to true, a preview is generated for each item. If \a show
     * is false, the MIME type icon of the item is shown instead. Per default showing
     * of the preview is turned on. Note that it is mandatory that the item view
     * specifies an icon size by QAbstractItemView::setIconSize(), otherwise
     * KFilePreviewGenerator::showPreview() will always return false.
     */
    void setShowPreview(bool show);
    bool showPreview() const;

    /**
     * Updates the previews for all already available items. Usually It is only
     * necessary to invoke this method when the icon size of the abstract item view
     * has been changed by QAbstractItemView::setIconSize().
     */
    void updatePreviews();

    /** Cancels all pending previews. */
    void cancelPreviews();

private:
    class Private;
    Private* const d; /// @internal
    Q_DISABLE_COPY(KFilePreviewGenerator)
    
    Q_PRIVATE_SLOT(d, void generatePreviews(const KFileItemList&))
    Q_PRIVATE_SLOT(d, void addToPreviewQueue(const KFileItem&, const QPixmap&))
    Q_PRIVATE_SLOT(d, void slotPreviewJobFinished(KJob*))
    Q_PRIVATE_SLOT(d, void updateCutItems())
    Q_PRIVATE_SLOT(d, void dispatchPreviewQueue())
    Q_PRIVATE_SLOT(d, void pausePreviews())
    Q_PRIVATE_SLOT(d, void resumePreviews())
};

#endif
