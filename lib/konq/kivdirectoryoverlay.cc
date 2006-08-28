/*  This file is part of the KDE libraries
    Copyright (C) 2002 Simon MacMullen

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <q3dict.h>
#include <QPixmap>
#include <QPainter>
#include <QBitmap>
#include <QImage>
//Added by qt3to4:
#include <QTimerEvent>

#include <kfileivi.h>
#include <kfileitem.h>
#include <kapplication.h>
#include <kdirlister.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <konq_settings.h>
#include <klocale.h>
#include <kdebug.h>

#include "kivdirectoryoverlay.h"

KIVDirectoryOverlay::KIVDirectoryOverlay(KFileIVI* directory)
: m_lister(0), m_foundItems(false),
  m_containsFolder(false), m_popularIcons(0)
{
    if (!m_lister)
    {
        m_lister = new KDirLister;
        m_lister->setAutoErrorHandlingEnabled(false, 0);
        connect(m_lister, SIGNAL(completed()), SLOT(slotCompleted()));
        connect(m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(slotNewItems( const KFileItemList& )));
        m_lister->setShowingDotFiles(false);
    }
    m_directory = directory;
}

KIVDirectoryOverlay::~KIVDirectoryOverlay()
{
    if (m_lister) m_lister->stop();
    delete m_lister;
    delete m_popularIcons;
}

void KIVDirectoryOverlay::start()
{
    if ( m_directory->item()->isReadable() ) {
        m_popularIcons = new Q3Dict<int>;
        m_popularIcons->setAutoDelete(true);
        m_lister->openUrl(m_directory->item()->url());
    } else {
        emit finished();
    }
}

void KIVDirectoryOverlay::timerEvent(QTimerEvent *)
{
    m_lister->stop();
}

void KIVDirectoryOverlay::slotCompleted()
{
    if (!m_popularIcons) return;

    // Look through the histogram for the most popular mimetype
    Q3DictIterator<int> currentIcon( (*m_popularIcons) );
    unsigned int best = 0;
    unsigned int total = 0;
    for ( ; currentIcon.current(); ++currentIcon ) {
        unsigned int currentCount = (*currentIcon.current());
        total += currentCount;
        if ( best < currentCount ) {
            best = currentCount;
            m_bestIcon = currentIcon.currentKey();
        }
    }

    // Only show folder if there's no other candidate. Most folders contain
    // folders. We know this.
    if ( m_bestIcon.isNull() && m_containsFolder ) {
        m_bestIcon = "folder";
    }
    
    if ( best * 2 < total ) {
        m_bestIcon = "kmultiple";
    }

    if (!m_bestIcon.isNull()) {
        m_directory->setOverlay(m_bestIcon);
    }

    delete m_popularIcons;
    m_popularIcons = 0;

    emit finished();
}

void KIVDirectoryOverlay::slotNewItems( const KFileItemList& items )
{
    if ( !m_popularIcons) return;

    KFileItemList::const_iterator it = items.begin();
    const KFileItemList::const_iterator end = items.end();
    for ( ; it != end; ++it ) {
        KFileItem* file = *it;
        if ( file->isFile() ) {
            QString iconName = file->iconName();
            if (iconName.isEmpty())
                continue;

            int* iconCount = m_popularIcons->find( file->iconName() );
            if (!iconCount) {
                iconCount = new int(0);
                Q_ASSERT(file);
                m_popularIcons->insert(file->iconName(), iconCount);
            }
            (*iconCount)++;
        } else if ( file->isDir() ) {
            m_containsFolder = true;
        }
    }

    m_foundItems = true;
}

#include "kivdirectoryoverlay.moc"
