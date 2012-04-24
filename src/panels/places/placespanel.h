/***************************************************************************
 *   Copyright (C) 2008-2012 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2010 by Christian Muehlhaeuser <muesli@gmail.com>       *
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

#ifndef PLACESPANEL_H
#define PLACESPANEL_H

#include <config-nepomuk.h>

#include <KUrl>
#include <panels/panel.h>
#include <QHash>
#include <QList>
#include <QSet>

class KBookmarkManager;
class KItemListController;
class KStandardItemModel;

#ifdef HAVE_NEPOMUK
namespace Nepomuk
{
    namespace Query
    {
        class Term;
    }
}
#endif

/**
 * @brief Combines bookmarks and mounted devices as list.
 */
class PlacesPanel : public Panel
{
    Q_OBJECT

public:
    PlacesPanel(QWidget* parent);
    virtual ~PlacesPanel();

signals:
    void placeActivated(const KUrl& url);
    void placeMiddleClicked(const KUrl& url);

protected:
    virtual bool urlChanged();
    virtual void showEvent(QShowEvent* event);

private slots:
    void slotItemActivated(int index);
    void slotItemMiddleClicked(int index);
    void slotItemContextMenuRequested(int index, const QPointF& pos);
    void slotViewContextMenuRequested(const QPointF& pos);
    void slotUrlsDropped(const KUrl& dest, QDropEvent* event, QWidget* parent);

private:
    void createDefaultBookmarks();
    void loadBookmarks();
    KUrl urlForIndex(int index) const;

    /**
     * @return URL using the timeline-protocol for searching.
     */
    static KUrl createTimelineUrl(const KUrl& url);

    /**
     * Helper method for createTimelineUrl().
     * @return String that represents a date-path in the format that
     *         the timeline-protocol expects.
     */
    static QString timelineDateString(int year, int month, int day = 0);

    /**
     * @return URL that can be listed by KIO and results in searching
     *         for a given term. The URL \a url represents a places-internal
     *         URL like e.g. "search:/documents"
     */
    static KUrl createSearchUrl(const KUrl& url);

#ifdef HAVE_NEPOMUK
    /**
     * Helper method for createSearchUrl().
     * @return URL that can be listed by KIO and results in searching
     *         for the given term.
     */
    static KUrl searchUrlForTerm(const Nepomuk::Query::Term& term);
#endif

private:
    bool m_nepomukRunning;

    KItemListController* m_controller;
    KStandardItemModel* m_model;

    QSet<QString> m_availableDevices;
    KBookmarkManager* m_bookmarkManager;

    struct DefaultBookmarkData
    {
        DefaultBookmarkData(const KUrl& url,
                            const QString& icon,
                            const QString& text,
                            const QString& group) :
            url(url), icon(icon), text(text), group(group) {}
        KUrl url;
        QString icon;
        QString text;
        QString group;
    };

    QList<DefaultBookmarkData> m_defaultBookmarks;
    QHash<KUrl, int> m_defaultBookmarksIndexes;
};

#endif // PLACESPANEL_H
