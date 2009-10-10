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

#ifndef _NEPOMUK_MASS_UPDATE_JOB_H_
#define _NEPOMUK_MASS_UPDATE_JOB_H_

#include <kjob.h>
#include <kurl.h>

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QTimer>

#include <nepomuk/resource.h>
#include <nepomuk/variant.h>


namespace Nepomuk {
    class MassUpdateJob : public KJob
    {
        Q_OBJECT

    public:
        MassUpdateJob( QObject* parent = 0 );
        ~MassUpdateJob();

        /**
         * Set a list of files to change
         * This has the same effect as using setResources
         * with a list of manually created resources.
         */
        void setFiles( const KUrl::List& urls );

        /**
         * Set a list of resources to change.
         */
        void setResources( const QList<Nepomuk::Resource>& );

        /**
         * Set the properties to change in the mass update.
         */
        void setProperties( const QList<QPair<QUrl,Nepomuk::Variant> >& props );

        /**
         * Actually start the job.
         */
        void start();

        static MassUpdateJob* tagResources( const QList<Nepomuk::Resource>&, const QList<Nepomuk::Tag>& tags );
        static MassUpdateJob* commentResources( const QList<Nepomuk::Resource>&, const QString& comment );
        static MassUpdateJob* rateResources( const QList<Nepomuk::Resource>&, int rating );

    protected:
        bool doKill();
        bool doSuspend();
        bool doResume();

    private Q_SLOTS:
        void slotNext();

    private:
        QList<Nepomuk::Resource> m_resources;
        QList<QPair<QUrl,Nepomuk::Variant> > m_properties;
        int m_index;
        QTimer m_processTimer;
    };
}

#endif
