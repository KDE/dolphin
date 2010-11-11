/***************************************************************************
 *   Copyright (C) 2010 by Sebastian Trueg <trueg@kde.org>                  *
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

#ifndef FILTERPANEL_H
#define FILTERPANEL_H

#include <nepomuk/query.h>
#include <panels/panel.h>

class KJob;

namespace Nepomuk {
    namespace Utils {
        class FacetWidget;
    }
}

/**
 * @brief Allows the filtering of search results.
 */
class FilterPanel : public Panel
{
    Q_OBJECT

public:
    FilterPanel(QWidget* parent = 0);
    virtual ~FilterPanel();

signals:
    void urlActivated(const KUrl& url);

protected:
    /** @see Panel::urlChanged() */
    virtual bool urlChanged();

    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

private slots:
    void slotSetUrlStatFinished(KJob*);
    void slotQueryTermChanged(const Nepomuk::Query::Term& term);

private:
    void setQuery(const Nepomuk::Query::Query& query);

private:
    bool m_initialized;
    bool m_nepomukEnabled;
    KJob* m_lastSetUrlStatJob;

    Nepomuk::Utils::FacetWidget* m_facetWidget;
    Nepomuk::Query::Query m_unfacetedRestQuery;
};

#endif // FILTERPANEL_H
