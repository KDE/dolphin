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

#include "facetpanel.h"

#include <nepomuk/filequery.h>
#include <nepomuk/facetwidget.h>
#include <Nepomuk/Query/FileQuery>
#include <Nepomuk/Query/Term>

#include <kfileitem.h>
#include <kio/jobclasses.h>
#include <kio/job.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QTreeView>
#include <QtGui/QPushButton>
#include <kdebug.h>


FacetPanel::FacetPanel(QWidget* parent)
    : Panel(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_buttonRemoveFolderRestriction = new QPushButton( i18n( "Remove folder restriction" ), this );
    connect( m_buttonRemoveFolderRestriction, SIGNAL( clicked() ), SLOT( slotRemoveFolderRestrictionClicked() ) );

    layout->addWidget(m_buttonRemoveFolderRestriction);

    m_facetWidget = new Nepomuk::Utils::FacetWidget( this );
    layout->addWidget( m_facetWidget, 1 );
    connect(m_facetWidget, SIGNAL(facetsChanged()), this, SLOT(slotFacetsChanged()) );

    // init to empty panel
    setQuery(Nepomuk::Query::Query());
}


FacetPanel::~FacetPanel()
{
}

void FacetPanel::setUrl(const KUrl& url)
{
    kDebug() << url;
    Panel::setUrl(url);

    // disable us
    setQuery(Nepomuk::Query::Query());

    // get the query from the item
    m_lastSetUrlStatJob = KIO::stat(url, KIO::HideProgressInfo);
    connect(m_lastSetUrlStatJob, SIGNAL(result(KJob*)),
            this, SLOT(slotSetUrlStatFinished(KJob*)));
}


void FacetPanel::setQuery(const Nepomuk::Query::Query& query)
{
    kDebug() << query << query.isValid() << query.toSparqlQuery();

    if (query.isValid()) {
        m_buttonRemoveFolderRestriction->setVisible( query.isFileQuery() && !query.toFileQuery().includeFolders().isEmpty() );
        m_unfacetedRestQuery = query;
        m_unfacetedRestQuery.setTerm( m_facetWidget->extractFacetsFromTerm( query.term() ) );
        m_facetWidget->setClientQuery( query );
        kDebug() << "Rest query after facets:" << m_unfacetedRestQuery;
        setEnabled(true);
    }
    else {
        m_unfacetedRestQuery = Nepomuk::Query::Query();
        setEnabled(false);
    }
}


void FacetPanel::slotSetUrlStatFinished(KJob* job)
{
    m_lastSetUrlStatJob = 0;
    kDebug() << url();
    const KIO::UDSEntry uds = static_cast<KIO::StatJob*>(job)->statResult();
    const QString nepomukQueryStr = uds.stringValue( KIO::UDSEntry::UDS_NEPOMUK_QUERY );
    kDebug() << nepomukQueryStr;
    Nepomuk::Query::FileQuery nepomukQuery;
    if ( !nepomukQueryStr.isEmpty() ) {
        nepomukQuery = Nepomuk::Query::Query::fromString( nepomukQueryStr );
    }
    else if ( url().isLocalFile() ) {
        // fallback query for local file URLs
        nepomukQuery.addIncludeFolder(url(), false);
    }
    kDebug() << nepomukQuery;
    setQuery(nepomukQuery);
}


void FacetPanel::slotFacetsChanged()
{
    Nepomuk::Query::Query query( m_unfacetedRestQuery && m_facetWidget->queryTerm() );
    kDebug() << query;
    emit urlActivated( query.toSearchUrl() );
}


void FacetPanel::slotRemoveFolderRestrictionClicked()
{
    Nepomuk::Query::FileQuery query( m_unfacetedRestQuery && m_facetWidget->queryTerm() );
    query.setIncludeFolders( KUrl::List() );
    query.setExcludeFolders( KUrl::List() );
    m_facetWidget->setClientQuery( query );
    emit urlActivated( query.toSearchUrl() );
}
