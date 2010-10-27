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

#include <QPushButton>
#include <QShowEvent>
#include <QTreeView>
#include <QVBoxLayout>

FilterPanel::FilterPanel(QWidget* parent) :
    Panel(parent),
    m_initialized(false),
    m_lastSetUrlStatJob(0),
    m_removeFolderRestrictionButton(0),
    m_facetWidget(0),
    m_unfacetedRestQuery()
{
}

FilterPanel::~FilterPanel()
{
}

bool FilterPanel::urlChanged()
{
    if (isVisible()) {
        setQuery(Nepomuk::Query::Query());

        delete m_lastSetUrlStatJob;

        m_lastSetUrlStatJob = KIO::stat(url(), KIO::HideProgressInfo);
        connect(m_lastSetUrlStatJob, SIGNAL(result(KJob*)),
                this, SLOT(slotSetUrlStatFinished(KJob*)));
    }

    return true;
}

void FilterPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (!m_initialized) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        Q_ASSERT(m_removeFolderRestrictionButton == 0);
        m_removeFolderRestrictionButton = new QPushButton(i18n("Remove folder restriction"), this);
        connect(m_removeFolderRestrictionButton, SIGNAL(clicked()), SLOT(slotRemoveFolderRestrictionClicked()));

        layout->addWidget(m_removeFolderRestrictionButton);

        Q_ASSERT(m_facetWidget == 0);
        m_facetWidget = new Nepomuk::Utils::FacetWidget(this);
        layout->addWidget(m_facetWidget, 1);

        m_facetWidget->addFacet(Nepomuk::Utils::Facet::createFileTypeFacet());
        m_facetWidget->addFacet(Nepomuk::Utils::Facet::createDateFacet());
        m_facetWidget->addFacet(Nepomuk::Utils::Facet::createRatingFacet());
        m_facetWidget->addFacet(Nepomuk::Utils::Facet::createTagFacet());

        Q_ASSERT(m_lastSetUrlStatJob == 0);
        m_lastSetUrlStatJob = KIO::stat(url(), KIO::HideProgressInfo);
        connect(m_lastSetUrlStatJob, SIGNAL(result(KJob*)),
                this, SLOT(slotSetUrlStatFinished(KJob*)));

        connect(m_facetWidget, SIGNAL(facetsChanged()), this, SLOT(slotFacetsChanged()));

        m_initialized = true;
    }

    Panel::showEvent(event);
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
    } else {
        m_unfacetedRestQuery = Nepomuk::Query::Query();
        setEnabled(false);
    }
}
