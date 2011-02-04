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

#include "dolphin_searchsettings.h"

#include <Nepomuk/ResourceManager>
#include <Nepomuk/Utils/FacetWidget>
#include <Nepomuk/Utils/Facet>
#include <Nepomuk/Utils/SimpleFacet>
#include <Nepomuk/Utils/ProxyFacet>
#include <Nepomuk/Utils/DynamicResourceFacet>
#include <Nepomuk/Query/FileQuery>
#include <Nepomuk/Query/ResourceTypeTerm>
#include <Nepomuk/Query/LiteralTerm>
#include <Nepomuk/Query/ComparisonTerm>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Vocabulary/NIE>

#include <search/dolphinsearchinformation.h>

#include <KFileItem>
#include <kio/jobclasses.h>
#include <KIO/Job>
#include <KMenu>

#include <QPushButton>
#include <QShowEvent>
#include <QTreeView>
#include <QVBoxLayout>

FilterPanel::FilterPanel(QWidget* parent) :
    Panel(parent),
    m_initialized(false),
    m_lastSetUrlStatJob(0),
    m_startedFromDir(),
    m_facetWidget(0),
    m_unfacetedRestQuery()
{
    setEnabled(false);
}

FilterPanel::~FilterPanel()
{
}

bool FilterPanel::urlChanged()
{
    if (!url().protocol().startsWith(QLatin1String("nepomuk"))) {
        // Remember the current directory before a searching is started.
        // This is required to restore the directory in case that all facets
        // have been reset by the user (see slotQueryTermChanged()).
        m_startedFromDir = url();
    }

    if (isVisible() && DolphinSearchInformation::instance().isIndexingEnabled()) {
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
        layout->setMargin(0);

        Q_ASSERT(m_facetWidget == 0);
        m_facetWidget = new Nepomuk::Utils::FacetWidget(this);
        layout->addWidget(m_facetWidget, 1);

        // File Type
        m_facetWidget->addFacet(Nepomuk::Utils::Facet::createFileTypeFacet());

        // Image Size
        Nepomuk::Utils::ProxyFacet* imageSizeProxy = new Nepomuk::Utils::ProxyFacet();
        imageSizeProxy->setFacetCondition(Nepomuk::Query::ResourceTypeTerm(Nepomuk::Vocabulary::NFO::Image()));
        Nepomuk::Utils::SimpleFacet* imageSizeFacet = new Nepomuk::Utils::SimpleFacet(imageSizeProxy);
        imageSizeFacet->setSelectionMode(Nepomuk::Utils::Facet::MatchAny);
        imageSizeFacet->addTerm( i18nc("option:check Refers to a filter on image size", "Small"),
                                Nepomuk::Vocabulary::NFO::width() <= Nepomuk::Query::LiteralTerm(300));
        imageSizeFacet->addTerm( i18nc("option:check Refers to a filter on image size", "Medium"),
                                (Nepomuk::Vocabulary::NFO::width() > Nepomuk::Query::LiteralTerm(300)) &&
                                (Nepomuk::Vocabulary::NFO::width() <= Nepomuk::Query::LiteralTerm(800)));
        imageSizeFacet->addTerm( i18nc("option:check Refers to a filter on image size", "Large"),
                                Nepomuk::Vocabulary::NFO::width() > Nepomuk::Query::LiteralTerm(800));
        imageSizeProxy->setSourceFacet(imageSizeFacet);
        m_facetWidget->addFacet(imageSizeProxy);

        // Artists
        Nepomuk::Utils::ProxyFacet* artistProxy = new Nepomuk::Utils::ProxyFacet();
        artistProxy->setFacetCondition(Nepomuk::Query::ResourceTypeTerm(Nepomuk::Vocabulary::NFO::Audio()) ||
                                       Nepomuk::Query::ComparisonTerm(Nepomuk::Vocabulary::NIE::mimeType(),
                                                                      Nepomuk::Query::LiteralTerm(QLatin1String("audio"))));
        Nepomuk::Utils::DynamicResourceFacet* artistFacet = new Nepomuk::Utils::DynamicResourceFacet(artistProxy);
        artistFacet->setSelectionMode(Nepomuk::Utils::Facet::MatchAny);
        artistFacet->setRelation(Nepomuk::Vocabulary::NMM::performer());
        artistProxy->setSourceFacet(artistFacet);
        m_facetWidget->addFacet(artistProxy);

        // Misc
        m_facetWidget->addFacet(Nepomuk::Utils::Facet::createDateFacet());
        m_facetWidget->addFacet(Nepomuk::Utils::Facet::createRatingFacet());
        m_facetWidget->addFacet(Nepomuk::Utils::Facet::createTagFacet());

        Q_ASSERT(m_lastSetUrlStatJob == 0);
        m_lastSetUrlStatJob = KIO::stat(url(), KIO::HideProgressInfo);
        connect(m_lastSetUrlStatJob, SIGNAL(result(KJob*)),
                this, SLOT(slotSetUrlStatFinished(KJob*)));

        connect(m_facetWidget, SIGNAL(queryTermChanged(Nepomuk::Query::Term)),
                this, SLOT(slotQueryTermChanged(Nepomuk::Query::Term)));

        m_initialized = true;
    }

    const DolphinSearchInformation& searchInfo = DolphinSearchInformation::instance();
    setEnabled(searchInfo.isIndexingEnabled() &&
               searchInfo.isPathIndexed(m_startedFromDir));

    Panel::showEvent(event);
}

void FilterPanel::hideEvent(QHideEvent* event)
{
    if (!event->spontaneous()) {
        setEnabled(false);
    }

    Panel::hideEvent(event);
}

void FilterPanel::contextMenuEvent(QContextMenuEvent* event)
{
    Panel::contextMenuEvent(event);

    QWeakPointer<KMenu> popup = new KMenu(this);
    foreach (QAction* action, customContextMenuActions()) {
        popup.data()->addAction(action);
    }
    popup.data()->exec(QCursor::pos());
    delete popup.data();
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
        // Fallback query for local file URLs: List all files
        Nepomuk::Query::ComparisonTerm compTerm(
                                Nepomuk::Vocabulary::NFO::fileName(),
                                Nepomuk::Query::Term());
        nepomukQuery.setFileMode(Nepomuk::Query::FileQuery::QueryFiles);
        if (SearchSettings::location() == QLatin1String("FromHere")) {
            nepomukQuery.addIncludeFolder(url(), true);
        }
        nepomukQuery.setTerm(compTerm);
    }
    setQuery(nepomukQuery);
}

void FilterPanel::slotQueryTermChanged(const Nepomuk::Query::Term& term)
{
    if (term.isValid()) {
        // Default case: A facet has been changed by the user to restrict the query.
        Nepomuk::Query::FileQuery query(m_unfacetedRestQuery && term);
        emit urlActivated(query.toSearchUrl());
        return;
    }

    // All facets have been reset by the user to be unrestricted.
    // Verify whether the unfaceted rest query contains any additional restriction
    // (e.g. a filename in the search field). If no further restriction is given, exit
    // the search mode by returning to the directory where the searching has been
    // started from.
    const Nepomuk::Query::Term rootTerm = m_unfacetedRestQuery.term();
    if (rootTerm.type() == Nepomuk::Query::Term::Comparison) {
        const Nepomuk::Query::ComparisonTerm& compTerm = static_cast<const Nepomuk::Query::ComparisonTerm&>(rootTerm);
        if (compTerm.subTerm().isValid()) {
            Nepomuk::Query::FileQuery query(m_unfacetedRestQuery);
            emit urlActivated(query.toSearchUrl());
            return;
        }
    }

    emit urlActivated(m_startedFromDir);
}

void FilterPanel::setQuery(const Nepomuk::Query::Query& query)
{
    if (query.isValid()) {
        const bool block = m_facetWidget->blockSignals(true);

        m_unfacetedRestQuery = m_facetWidget->extractFacetsFromQuery(query);
        m_facetWidget->setClientQuery(query);

        const DolphinSearchInformation& searchInfo = DolphinSearchInformation::instance();
        setEnabled(searchInfo.isIndexingEnabled() &&
                   searchInfo.isPathIndexed(m_startedFromDir));

        m_facetWidget->blockSignals(block);
    } else {
        m_unfacetedRestQuery = Nepomuk::Query::Query();
        setEnabled(false);
    }
}
