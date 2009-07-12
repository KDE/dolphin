/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "revisioncontrolobserver.h"

#include "dolphinmodel.h"
#include "revisioncontrolplugin.h"

#include <kdirlister.h>

#include <QAbstractProxyModel>
#include <QAbstractItemView>
#include <QTimer>

RevisionControlObserver::RevisionControlObserver(QAbstractItemView* view) :
    QObject(view),
    m_view(view),
    m_dirLister(0),
    m_dolphinModel(0),
    m_dirVerificationTimer(0),
    m_plugin(0)
{
    Q_ASSERT(view != 0);

    QAbstractProxyModel* proxyModel = qobject_cast<QAbstractProxyModel*>(view->model());
    m_dolphinModel = (proxyModel == 0) ?
                 qobject_cast<DolphinModel*>(view->model()) :
                 qobject_cast<DolphinModel*>(proxyModel->sourceModel());
    if (m_dolphinModel != 0) {
        m_dirLister = m_dolphinModel->dirLister();
        connect(m_dirLister, SIGNAL(completed()),
                this, SLOT(delayedDirectoryVerification()));
        // TODO:
        // connect(m_dirLister, SIGNAL(refreshItems(const QList<QPair<KFileItem,KFileItem>>&)),
        //        this, SLOT(refreshItems()));

        // The verification timer specifies the timeout until the shown directory
        // is checked whether it is versioned. Per default it is assumed that users
        // don't iterate through versioned directories and a high timeout is used
        // The timeout will be decreased as soon as a versioned directory has been
        // found (see verifyDirectory()).
        m_dirVerificationTimer = new QTimer(this);
        m_dirVerificationTimer->setSingleShot(true);
        m_dirVerificationTimer->setInterval(500);
        connect(m_dirVerificationTimer, SIGNAL(timeout()),
                this, SLOT(verifyDirectory()));
    }
}

RevisionControlObserver::~RevisionControlObserver()
{
    delete m_plugin;
    m_plugin = 0;
}

void RevisionControlObserver::delayedDirectoryVerification()
{
    m_dirVerificationTimer->start();
}

void RevisionControlObserver::verifyDirectory()
{
    KUrl revisionControlUrl = m_dirLister->url();
    if (!revisionControlUrl.isLocalFile()) {
        return;
    }

    if (m_plugin == 0) {
        // TODO: just for testing purposes. A plugin approach will be used later.
        m_plugin = new SubversionPlugin();
    }

    revisionControlUrl.addPath(m_plugin->fileName());
    KFileItem item = m_dirLister->findByUrl(revisionControlUrl);
    if (item.isNull()) {
        // The directory is not versioned. Reset the verification timer to a higher
        // value, so that browsing through non-versioned directories is not slown down
        // by an immediate verification.
        m_dirVerificationTimer->setInterval(500);
    } else {
        // The directory is versioned. Assume that the user will further browse through
        // versioned directories and decrease the verification timer.
        m_dirVerificationTimer->setInterval(100);
        updateItemStates();
    }
}

void RevisionControlObserver::updateItemStates()
{
    Q_ASSERT(m_plugin != 0);
    const KUrl directory = m_dirLister->url();
    if (!m_plugin->beginRetrieval(directory.toLocalFile(KUrl::AddTrailingSlash))) {
        return;
    }

    const int rowCount = m_dolphinModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        const QModelIndex index = m_dolphinModel->index(row, DolphinModel::Revision);
        const KFileItem item = m_dolphinModel->itemForIndex(index);
        const RevisionControlPlugin::RevisionState revision = m_plugin->revisionState(item.name());
        m_dolphinModel->setData(index, QVariant(static_cast<int>(revision)), Qt::DecorationRole);
    }
    m_view->viewport()->repaint(); // TODO: this should not be necessary, as DolphinModel::setData() calls dataChanged()

    m_plugin->endRetrieval();
}

#include "revisioncontrolobserver.moc"
