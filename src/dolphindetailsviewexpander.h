/***************************************************************************
 *   Copyright (C) 2009 by Frank Reininghaus (frank78ac@googlemail.com)    *
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

#ifndef DOLPHINDETAILSVIEWEXPANDER_H
#define DOLPHINDETAILSVIEWEXPANDER_H

#include <QObject>
#include <QSet>
#include <QList>

class DolphinDetailsView;
class KUrl;
class KDirLister;
class DolphinModel;
class DolphinSortFilterProxyModel;

/**
 * @brief Expands a given set of subfolders in collaboration with the dir lister and the dir model.
 *
 * Note that only one subfolder can be expanded at a time. Each expansion triggers KDirLister::openUrl(...),
 * and further expansions can only be done the next time the dir lister emits its completed() signal.
 */
class DolphinDetailsViewExpander : public QObject
{
    Q_OBJECT

public:
    explicit DolphinDetailsViewExpander(DolphinDetailsView* parent,
                                        const QSet<KUrl>& urlsToExpand);

    virtual ~DolphinDetailsViewExpander();

    /**
     * Stops the expansion and deletes the object via deleteLater().
     */
    void stop();

private slots:
    /**
     * This slot is invoked every time the dir lister has completed a listing.
     * It expands the first URL from the list m_urlsToExpand that can be found in the dir model.
     * If the list is empty, stop() is called.
     */
    void slotDirListerCompleted();

signals:
    /**
     * Is emitted when the expander has finished expanding URLs in the details view.
     */
    void completed();

private:
    QList<KUrl> m_urlsToExpand;

    DolphinDetailsView* m_detailsView;
    const KDirLister* m_dirLister;
    const DolphinModel* m_dolphinModel;
    const DolphinSortFilterProxyModel* m_proxyModel;
};

#endif
