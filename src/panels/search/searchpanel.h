/***************************************************************************
 *   Copyright (C) 2010 by Sebastian Trueg <trueg@kde.org>                 *
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

#ifndef SEARCHPANEL_H
#define SEARCHPANEL_H

#include <Nepomuk/Query/Query>
#include <panels/panel.h>

namespace KIO
{
    class Job;
};

namespace Nepomuk
{
    namespace Utils
    {
        class FacetWidget;
    }
}

/**
 * @brief Allows to search for files by enabling generic search patterns (= facets).
 *
 * For example it is possible to search for images, documents or specific tags.
 * The search panel can be adjusted to search only from the current folder or everywhere.
 */
class SearchPanel : public Panel
{
    Q_OBJECT

public:
    enum SearchLocation
    {
        Everywhere,
        FromCurrentDir
    };

    SearchPanel(QWidget* parent = 0);
    virtual ~SearchPanel();

    /**
     * Specifies whether a searching is done in all folders (= Everywhere)
     * or from the current directory (= FromCurrentDir). The current directory
     * is automatically determined when setUrl() has been called.
     */
    void setSearchLocation(SearchLocation location);
    SearchLocation searchLocation() const;

signals:
    void urlActivated(const KUrl& url);

protected:
    /** @see Panel::urlChanged() */
    virtual bool urlChanged();

    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

    /** @see QWidget::contextMenuEvent() */
    virtual void contextMenuEvent(QContextMenuEvent* event);

private slots:
    void slotSetUrlStatFinished(KJob*);
    void slotQueryTermChanged(const Nepomuk::Query::Term& term);

private:
    void setQuery(const Nepomuk::Query::Query& query);

    /**
     * @return True if the facets can be applied to the given URL
     *         and hence a filtering of the content is possible.
     *         False is returned if the search-mode is set to
     *         SearchLocation::FromCurrentDir and this directory is
     *         not indexed at all. Also if indexing is disabled
     *         false will be returned.
     */
    bool isFilteringPossible() const;

private:
    bool m_initialized;
    SearchLocation m_searchLocation;
    KIO::Job* m_lastSetUrlStatJob;

    KUrl m_startedFromDir;
    Nepomuk::Utils::FacetWidget* m_facetWidget;
    Nepomuk::Query::Query m_unfacetedRestQuery;
};

#endif // SEARCHPANEL_H
