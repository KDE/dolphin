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

#include "viewextensionsfactory.h"

#include "dolphincontroller.h"
#include "dolphinfileitemdelegate.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"
#include "dolphinviewautoscroller.h"
#include "selectionmanager.h"
#include "settings/dolphinsettings.h"
#include "tooltips/tooltipmanager.h"

#include "dolphin_generalsettings.h"

#include <kdirlister.h>
#include <kdirmodel.h>
#include <kfilepreviewgenerator.h>
#include <QAbstractItemView>

ViewExtensionsFactory::ViewExtensionsFactory(QAbstractItemView* view,
                                             DolphinController* controller) :
    QObject(view),
    m_view(view),
    m_controller(controller),
    m_toolTipManager(0),
    m_previewGenerator(0),
    m_selectionManager(0),
    m_autoScroller(0),
    m_fileItemDelegate(0)
{   
    view->setSelectionMode(QAbstractItemView::ExtendedSelection);

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    // initialize tooltips
    if (settings->showToolTips()) {
        DolphinSortFilterProxyModel* proxyModel = static_cast<DolphinSortFilterProxyModel*>(view->model());
        m_toolTipManager = new ToolTipManager(view, proxyModel);

        connect(controller, SIGNAL(hideToolTip()),
                m_toolTipManager, SLOT(hideTip()));
    }

    // initialize preview generator
    m_previewGenerator = new KFilePreviewGenerator(view);
    m_previewGenerator->setPreviewShown(controller->dolphinView()->showPreview());
    connect(controller, SIGNAL(zoomLevelChanged(int)),
            this, SLOT(slotZoomLevelChanged()));
    connect(controller, SIGNAL(cancelPreviews()),
            this, SLOT(cancelPreviews()));
    connect(controller->dolphinView(), SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));

    // initialize selection manager
    if (settings->showSelectionToggle()) {
        m_selectionManager = new SelectionManager(view);
        connect(m_selectionManager, SIGNAL(selectionChanged()),
                this, SLOT(requestActivation()));
        connect(controller, SIGNAL(urlChanged(const KUrl&)),
                m_selectionManager, SLOT(reset()));
    }

    // initialize auto scroller
    m_autoScroller = new DolphinViewAutoScroller(view);

    // initialize file item delegate
    m_fileItemDelegate = new DolphinFileItemDelegate(view);
    m_fileItemDelegate->setShowToolTipWhenElided(false);
    view->setItemDelegate(m_fileItemDelegate);

    // react on view property changes
    const DolphinView* dolphinView = controller->dolphinView();
    connect(dolphinView, SIGNAL(showHiddenFilesChanged()),
            this, SLOT(slotShowHiddenFilesChanged()));
    connect(dolphinView, SIGNAL(sortingChanged(DolphinView::Sorting)),
            this, SLOT(slotSortingChanged(DolphinView::Sorting)));
    connect(dolphinView, SIGNAL(sortOrderChanged(Qt::SortOrder)),
            this, SLOT(slotSortOrderChanged(Qt::SortOrder)));
    connect(dolphinView, SIGNAL(sortFoldersFirstChanged(bool)),
            this, SLOT(slotSortFoldersFirstChanged(bool)));

    connect(controller, SIGNAL(nameFilterChanged(const QString&)),
            this, SLOT(slotNameFilterChanged(const QString&)));

    view->viewport()->installEventFilter(this);
}

ViewExtensionsFactory::~ViewExtensionsFactory()
{
}

void ViewExtensionsFactory::handleCurrentIndexChange(const QModelIndex& current, const QModelIndex& previous)
{
    m_autoScroller->handleCurrentIndexChange(current, previous);
}

DolphinFileItemDelegate* ViewExtensionsFactory::fileItemDelegate() const
{
    return m_fileItemDelegate;
}

bool ViewExtensionsFactory::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);
    if ((event->type() == QEvent::Wheel) && (m_selectionManager != 0)) {
        m_selectionManager->reset();
    }
    return false;
}

void ViewExtensionsFactory::slotZoomLevelChanged()
{
    m_previewGenerator->updateIcons();
    if (m_selectionManager != 0) {
        m_selectionManager->reset();
    }
}

void ViewExtensionsFactory::cancelPreviews()
{
    m_previewGenerator->cancelPreviews();
}

void ViewExtensionsFactory::slotShowPreviewChanged()
{
    const bool show = m_controller->dolphinView()->showPreview();
    m_previewGenerator->setPreviewShown(show);
}

void ViewExtensionsFactory::slotShowHiddenFilesChanged()
{
    KDirModel* dirModel = static_cast<KDirModel*>(proxyModel()->sourceModel());
    KDirLister* dirLister = dirModel->dirLister();

    dirLister->stop();

    const bool show = m_controller->dolphinView()->showHiddenFiles();
    dirLister->setShowingDotFiles(show);

    const KUrl url = dirLister->url();
    if (url.isValid()) {
        dirLister->openUrl(url, KDirLister::NoFlags);
    }
}

void ViewExtensionsFactory::slotSortingChanged(DolphinView::Sorting sorting)
{
    proxyModel()->setSorting(sorting);
}

void ViewExtensionsFactory::slotSortOrderChanged(Qt::SortOrder order)
{
    proxyModel()->setSortOrder(order);
}

void ViewExtensionsFactory::slotSortFoldersFirstChanged(bool foldersFirst)
{
    proxyModel()->setSortFoldersFirst(foldersFirst);
}

void ViewExtensionsFactory::slotNameFilterChanged(const QString& nameFilter)
{
    proxyModel()->setFilterRegExp(nameFilter);
}

void ViewExtensionsFactory::requestActivation()
{
    m_controller->requestActivation();
}

DolphinSortFilterProxyModel* ViewExtensionsFactory::proxyModel() const
{
    return static_cast<DolphinSortFilterProxyModel*>(m_view->model());
}

#include "viewextensionsfactory.moc"

