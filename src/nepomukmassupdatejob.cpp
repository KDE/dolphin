/***************************************************************************
 *   Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                 *
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

#include "nepomukmassupdatejob.h"

#include <klocale.h>
#include <kdebug.h>

#include <nepomuk/tag.h>
#include <nepomuk/tools.h>


Nepomuk::MassUpdateJob::MassUpdateJob( QObject* parent )
    : KJob( parent ),
      m_index( -1 )
{
    kDebug();
    setCapabilities( Killable|Suspendable );
    connect( &m_processTimer, SIGNAL( timeout() ),
             this, SLOT( slotNext() ) );
}


Nepomuk::MassUpdateJob::~MassUpdateJob()
{
    kDebug();
}


void Nepomuk::MassUpdateJob::setFiles( const KUrl::List& urls )
{
    m_resources.clear();
    foreach( KUrl url, urls ) {
        m_resources.append( Resource( url ) );
    }
    setTotalAmount( KJob::Files, m_resources.count() );
}


void Nepomuk::MassUpdateJob::setResources( const QList<Nepomuk::Resource>& rl )
{
    m_resources = rl;
    setTotalAmount( KJob::Files, m_resources.count() );
}


void Nepomuk::MassUpdateJob::setProperties( const QList<QPair<QUrl,Nepomuk::Variant> >& props )
{
    m_properties = props;
}


void Nepomuk::MassUpdateJob::start()
{
    if ( m_index < 0 ) {
        kDebug();
        emit description( this,
                          i18n("Changing annotations") );
        m_index = 0;
        m_processTimer.start();
    }
    else {
        kDebug() << "Job has already been started";
    }
}


bool Nepomuk::MassUpdateJob::doKill()
{
    if ( m_index > 0 ) {
        m_processTimer.stop();
        m_index = -1;
        return true;
    }
    else {
        return false;
    }
}


bool Nepomuk::MassUpdateJob::doSuspend()
{
    m_processTimer.stop();
    return true;
}


bool Nepomuk::MassUpdateJob::doResume()
{
    if ( m_index > 0 ) {
        m_processTimer.start();
        return true;
    }
    else {
        return false;
    }
}


void Nepomuk::MassUpdateJob::slotNext()
{
    if ( !isSuspended() ) {
        if ( m_index < m_resources.count() ) {
            Nepomuk::Resource& res = m_resources[m_index];
            for ( int i = 0; i < m_properties.count(); ++i ) {
                res.setProperty( m_properties[i].first, m_properties[i].second );
            }
            ++m_index;
            setProcessedAmount( KJob::Files, m_index );
        }
        else if ( m_index >= m_resources.count() ) {
            kDebug() << "done";
            m_index = -1;
            m_processTimer.stop();
            emitResult();
        }
    }
}


Nepomuk::MassUpdateJob* Nepomuk::MassUpdateJob::tagResources( const QList<Nepomuk::Resource>& rl, const QList<Nepomuk::Tag>& tags )
{
    Nepomuk::MassUpdateJob* job = new Nepomuk::MassUpdateJob();
    job->setResources( rl );
    job->setProperties( QList<QPair<QUrl,Nepomuk::Variant> >() << qMakePair( QUrl( Nepomuk::Resource::tagUri() ), Nepomuk::Variant( convertResourceList<Tag>( tags ) ) ) );
    return job;
}


Nepomuk::MassUpdateJob* Nepomuk::MassUpdateJob::rateResources( const QList<Nepomuk::Resource>& rl, int rating )
{
    Nepomuk::MassUpdateJob* job = new Nepomuk::MassUpdateJob();
    job->setResources( rl );
    job->setProperties( QList<QPair<QUrl,Nepomuk::Variant> >() << qMakePair( QUrl( Nepomuk::Resource::ratingUri() ), Nepomuk::Variant( rating ) ) );
    return job;
}


Nepomuk::MassUpdateJob* Nepomuk::MassUpdateJob::commentResources( const QList<Nepomuk::Resource>& rl, const QString& comment )
{
    Nepomuk::MassUpdateJob* job = new Nepomuk::MassUpdateJob();
    job->setResources( rl );
    job->setProperties( QList<QPair<QUrl,Nepomuk::Variant> >() << qMakePair( QUrl( Nepomuk::Resource::descriptionUri() ), Nepomuk::Variant( comment ) ) );
    return job;
}

#include "nepomukmassupdatejob.moc"
