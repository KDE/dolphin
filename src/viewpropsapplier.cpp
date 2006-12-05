/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "viewpropsapplier.h"
#include "viewproperties.h"

#include <assert.h>
#include <kurl.h>
#include <kfileitem.h>

ViewPropsApplier::ViewPropsApplier(const KUrl& dir, const ViewProperties* viewProps)
{
    m_rootData = new RootData();
    m_rootData->refCount = 0;
    m_rootData->viewProps = viewProps;
    m_rootData->rootApplier = this;

    start(dir);
}

ViewPropsApplier::~ViewPropsApplier()
{
    assert(m_rootData != 0);
    if (m_rootData->refCount == 0) {
        m_rootData->rootApplier->emitCompleted();
        delete m_rootData;
        m_rootData = 0;
    }

    delete m_dirLister;
    m_dirLister = 0;
}

void ViewPropsApplier::slotCompleted(const KUrl& /*dir*/)
{
    QTimer::singleShot(0, this, SLOT(countSubDirs()));
}

void ViewPropsApplier::countSubDirs()
{
    KFileItemList list = m_dirLister->items();
    const int dirCount = list.count();
    if (dirCount > 0) {
        m_rootData->rootApplier->emitProgress(m_dirLister->url(), dirCount);

        KFileItemList::const_iterator end = list.end();
        for (KFileItemList::const_iterator it = list.begin(); it != end; ++it) {
            assert((*it)->isDir());
            const KUrl& subDir = (*it)->url();
            if (m_rootData->viewProps != 0) {
                // TODO: provide copy constructor in ViewProperties
                ViewProperties props(subDir);
                props.setViewMode(m_rootData->viewProps->viewMode());
                props.setShowHiddenFilesEnabled(m_rootData->viewProps->sorting());
                props.setSorting(m_rootData->viewProps->sorting());
                props.setSortOrder(m_rootData->viewProps->sortOrder());
            }
            new ViewPropsApplier(subDir, m_rootData);
        }
    }

    --(m_rootData->refCount);
    if ((m_rootData->refCount == 0) || (m_rootData->rootApplier != this)) {
        delete this;
    }
}

ViewPropsApplier::ViewPropsApplier(const KUrl& dir,
                                   RootData* rootData)
{
    m_rootData = rootData;
    assert(m_rootData != 0);
    start(dir);
}

void ViewPropsApplier::start(const KUrl& dir)
{
    m_dirLister = new KDirLister();
    m_dirLister->setDirOnlyMode(true);
    m_dirLister->setAutoUpdate(false);
    connect(m_dirLister, SIGNAL(completed(const KUrl&)),
            this, SLOT(slotCompleted(const KUrl&)));
    m_dirLister->openUrl(dir);
    ++(m_rootData->refCount);
}

void ViewPropsApplier::emitProgress(const KUrl& dir, int count)
{
    emit progress(dir, count);
}

void ViewPropsApplier::emitCompleted()
{
    emit completed();
}

#include "viewpropsapplier.moc"
