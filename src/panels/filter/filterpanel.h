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

#include "../panel.h"
#include <nepomuk/query.h>

class KJob;
class QPushButton;

namespace Nepomuk {
    namespace Utils {
        class FacetWidget;
    }
}

class FilterPanel : public Panel
{
    Q_OBJECT

public:
    FilterPanel(QWidget* parent = 0);
    ~FilterPanel();

public slots:
    void setUrl(const KUrl& url);
    void setQuery(const Nepomuk::Query::Query& query);

signals:
    void urlActivated( const KUrl& url );

private slots:
    void slotSetUrlStatFinished(KJob*);
    void slotFacetsChanged();
    void slotRemoveFolderRestrictionClicked();

private:
    bool urlChanged() {
        return true;
    }

    KJob* m_lastSetUrlStatJob;

    QPushButton* m_buttonRemoveFolderRestriction;
    Nepomuk::Utils::FacetWidget* m_facetWidget;
    Nepomuk::Query::Query m_unfacetedRestQuery;
};

#endif // FILTERPANEL_H
