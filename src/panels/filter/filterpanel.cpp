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

#include "filterpanel.h"

#include <nepomuk/filequery.h>
#include <nepomuk/facetwidget.h>
#include <nepomuk/facet.h>
#include <Nepomuk/Query/FileQuery>
#include <Nepomuk/Query/Term>

#include <kfileitem.h>
#include <kio/jobclasses.h>
#include <kio/job.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QTreeView>
#include <QtGui/QPushButton>

FilterPanel::FilterPanel(QWidget* parent) :
    Panel(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_removeFolderRestrictionButton = new QPushButton(i18n("Remove folder restriction"), this);
    connect(m_removeFolderRestrictionButton, SIGNAL(clicked()), SLOT(slotRemoveFolderRestrictionClicked()));

    layout->addWidget(m_removeFolderRestrictionButton);

    m_facetWidget = new Nepomuk::Utils::FacetWidget(this);
    layout->addWidget(m_facetWidget, 1);
    connect(m_facetWidget, SIGNAL(facetsChanged()), this, SLOT(slotFacetsChanged()));

    /*m_facetWidget->addFacet(Nepomuk::Utils::Facet::createFileTypeFacet());
    m_facetWidget->addFacet(Nepomuk::Utils::Facet::createTypeFacet());
    m_facetWidget->addFacet(Nepomuk::Utils::Facet::createDateFacet());
    m_facetWidget->addFacet(Nepomuk::Utils::Facet::createPriorityFacet());
    m_facetWidget->addFacet(Nepomuk::Utils::Facet::createRatingFacet());*/

    // Init to empty panel
    setQuery(Nepomuk::Query::Query());
}

FilterPanel::~FilterPanel()
{
}

bool FilterPanel::urlChanged()
{
    if (!isVisible()) {
        return true;
    }

    // Disable us
    setQuery(Nepomuk::Query::Query());

    // Get the query from the item
    m_lastSetUrlStatJob = KIO::stat(url(), KIO::HideProgressInfo);
    connect(m_lastSetUrlStatJob, SIGNAL(result(KJob*)),
            this, SLOT(slotSetUrlStatFinished(KJob*)));

    return true;
}

void FilterPanel::slotSetUrlStatFinished(KJob* job)
{
    m_lastSetUrlStatJob = 0;
    const KIO::UDSEntry uds = static_cast<KIO::StatJob*>(job)->statResult();
    const QString nepomukQueryStr = uds.stringValue(KIO::UDSEntry::UDS_NEPOMUK_QUERY);
    Nepomuk::Query::FileQuery nepomukQuery;
    if (!nepomukQueryStr.isEmpty()) {
        nepomukQuery = Nepomuk::Query::Query::fromString(nepomukQueryStr);
    } else if (url().isLocalFile()) {
        // Fallback query for local file URLs
        nepomukQuery.addIncludeFolder(url(), false);
    }
    setQuery(nepomukQuery);
}

void FilterPanel::slotFacetsChanged()
{
    Nepomuk::Query::Query query(m_unfacetedRestQuery && m_facetWidget->queryTerm());
    emit urlActivated(query.toSearchUrl());
}

void FilterPanel::slotRemoveFolderRestrictionClicked()
{
    Nepomuk::Query::FileQuery query(m_unfacetedRestQuery && m_facetWidget->queryTerm());
    query.setIncludeFolders(KUrl::List());
    query.setExcludeFolders(KUrl::List());
    m_facetWidget->setClientQuery(query);
    emit urlActivated(query.toSearchUrl());
}

void FilterPanel::setQuery(const Nepomuk::Query::Query& query)
{
    if (query.isValid()) {
        m_removeFolderRestrictionButton->setVisible(query.isFileQuery() && !query.toFileQuery().includeFolders().isEmpty());
        m_unfacetedRestQuery = query;
        m_unfacetedRestQuery.setTerm(m_facetWidget->extractFacetsFromTerm(query.term()));
        m_facetWidget->setClientQuery(query);
        setEnabled(true);
    }
    else {
        m_unfacetedRestQuery = Nepomuk::Query::Query();
        setEnabled(false);
    }
}
