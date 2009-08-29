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
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"
#include "settings/dolphinsettings.h"
#include "tooltips/tooltipmanager.h"

#include "dolphin_generalsettings.h"

#include <kfilepreviewgenerator.h>
#include <QAbstractItemView>

ViewExtensionsFactory::ViewExtensionsFactory(QAbstractItemView* view,
                                             DolphinController* controller) :
    QObject(view),
    m_controller(controller),
    m_toolTipManager(0),
    m_previewGenerator(0)
{
    if (DolphinSettings::instance().generalSettings()->showToolTips()) {
        DolphinSortFilterProxyModel* proxyModel = static_cast<DolphinSortFilterProxyModel*>(view->model());
        m_toolTipManager = new ToolTipManager(view, proxyModel);

        connect(controller, SIGNAL(hideToolTip()),
                m_toolTipManager, SLOT(hideTip()));
    }

    m_previewGenerator = new KFilePreviewGenerator(view);
    m_previewGenerator->setPreviewShown(controller->dolphinView()->showPreview());
    connect(controller, SIGNAL(zoomLevelChanged(int)),
            this, SLOT(updateIcons()));
    connect(controller, SIGNAL(cancelPreviews()),
            this, SLOT(cancelPreviews()));
    connect(controller->dolphinView(), SIGNAL(showPreviewChanged()),
            this, SLOT(slotShowPreviewChanged()));
}

ViewExtensionsFactory::~ViewExtensionsFactory()
{
}

void ViewExtensionsFactory::updateIcons()
{
    m_previewGenerator->updateIcons();
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

#include "viewextensionsfactory.moc"

