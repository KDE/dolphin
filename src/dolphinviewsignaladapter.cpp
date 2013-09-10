/***************************************************************************
 * Copyright (C) 2013 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
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

#include "dolphinviewsignaladapter.h"

#include "dolphinviewcontainer.h"
#include "views/dolphinview.h"

#include <KUrlNavigator>

#include <QPoint>
#include <QAction>

DolphinViewSignalAdapter::DolphinViewSignalAdapter(QObject* parent) :
    QObject(parent),
    m_viewContainer(0)
{
}

void DolphinViewSignalAdapter::setViewContainer(DolphinViewContainer* container)
{
    if (m_viewContainer != container) {
        const DolphinViewContainer* oldContainer = m_viewContainer;
        m_viewContainer = container;

        if (oldContainer) {
            // Disconnect alls signals from the old view container.

            disconnect(oldContainer, SIGNAL(showFilterBarChanged(bool)),
                       this, SIGNAL(showFilterBarChanged(bool)));
            disconnect(oldContainer, SIGNAL(writeStateChanged(bool)),
                       this, SIGNAL(writeStateChanged(bool)));

            const DolphinView* oldView = oldContainer->view();
            disconnect(oldView, SIGNAL(selectionChanged(KFileItemList)),
                       this, SIGNAL(selectionChanged(KFileItemList)));
            disconnect(oldView, SIGNAL(requestItemInfo(KFileItem)),
                       this, SIGNAL(requestItemInfo(KFileItem)));
            disconnect(oldView, SIGNAL(directoryLoadingStarted()),
                       this, SIGNAL(directoryLoadingStarted()));
            disconnect(oldView, SIGNAL(directoryLoadingCompleted()),
                       this, SIGNAL(directoryLoadingCompleted()));
            disconnect(oldView, SIGNAL(goBackRequested()),
                       this, SIGNAL(goBackRequested()));
            disconnect(oldView, SIGNAL(goForwardRequested()),
                       this, SIGNAL(goForwardRequested()));
            disconnect(oldView, SIGNAL(requestContextMenu(QPoint,KFileItem,KUrl,QList<QAction*>)),
                       this, SIGNAL(requestContextMenu(QPoint,KFileItem,KUrl,QList<QAction*>)));
            disconnect(oldView, SIGNAL(tabRequested(KUrl)),
                       this, SIGNAL(tabRequested(KUrl)));
            disconnect(oldView, SIGNAL(urlChanged(KUrl)),
                       this, SIGNAL(urlChanged(KUrl)));

            const KUrlNavigator* oldUrlNavigator = oldContainer->urlNavigator();
            disconnect(oldUrlNavigator, SIGNAL(historyChanged()),
                       this, SIGNAL(historyChanged()));
            disconnect(oldUrlNavigator, SIGNAL(editableStateChanged(bool)),
                       this, SIGNAL(editableStateChanged(bool)));
            disconnect(oldUrlNavigator, SIGNAL(tabRequested(KUrl)),
                       this, SIGNAL(tabRequested(KUrl)));
        }

        if (container) {
            // Connect all signals from the new view container.

            connect(container, SIGNAL(showFilterBarChanged(bool)),
                    this, SIGNAL(showFilterBarChanged(bool)));
            connect(container, SIGNAL(writeStateChanged(bool)),
                    this, SIGNAL(writeStateChanged(bool)));

            const DolphinView* view = container->view();
            connect(view, SIGNAL(selectionChanged(KFileItemList)),
                    this, SIGNAL(selectionChanged(KFileItemList)));
            connect(view, SIGNAL(requestItemInfo(KFileItem)),
                    this, SIGNAL(requestItemInfo(KFileItem)));
            connect(view, SIGNAL(directoryLoadingStarted()),
                    this, SIGNAL(directoryLoadingStarted()));
            connect(view, SIGNAL(directoryLoadingCompleted()),
                    this, SIGNAL(directoryLoadingCompleted()));
            connect(view, SIGNAL(goBackRequested()),
                    this, SIGNAL(goBackRequested()));
            connect(view, SIGNAL(goForwardRequested()),
                    this, SIGNAL(goForwardRequested()));
            connect(view, SIGNAL(requestContextMenu(QPoint,KFileItem,KUrl,QList<QAction*>)),
                    this, SIGNAL(requestContextMenu(QPoint,KFileItem,KUrl,QList<QAction*>)));
            connect(view, SIGNAL(tabRequested(KUrl)),
                    this, SIGNAL(tabRequested(KUrl)));
            connect(view, SIGNAL(urlChanged(KUrl)),
                    this, SIGNAL(urlChanged(KUrl)));

            const KUrlNavigator* urlNavigator = container->urlNavigator();
            connect(urlNavigator, SIGNAL(historyChanged()),
                    this, SIGNAL(historyChanged()));
            connect(urlNavigator, SIGNAL(editableStateChanged(bool)),
                    this, SIGNAL(editableStateChanged(bool)));
            connect(urlNavigator, SIGNAL(tabRequested(KUrl)),
                    this, SIGNAL(tabRequested(KUrl)));
        }
    }
}
