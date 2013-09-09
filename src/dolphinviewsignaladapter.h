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

#ifndef DOLPHIN_VIEW_SIGNAL_ADAPTER_H
#define DOLPHIN_VIEW_SIGNAL_ADAPTER_H

#include <QObject>

class DolphinViewContainer;
class KUrl;
class KFileItem;
class KFileItemList;
class QAction;
class QPoint;

/**
 * Connects the signals from a DolphinViewContainer with the
 * corresponding signals of his own, but before it disconnects
 * all signals between him and the previous DolphinViewContainer.
 *
 * Used to decouple DolphinMainWindow and DolphinViewContainer signal-
 * slot-connections. The DolphinMainWindow needs only connect to the
 * signals of this class.
 *
 * It guarantees that the signal receiver of this class gets only the signals
 * from the active view container.
 */
class DolphinViewSignalAdapter : public QObject
{
    Q_OBJECT

public:
    explicit DolphinViewSignalAdapter(QObject* parent = 0);

    /*
     * Set the new view container, whose signals shoud be
     * forwarded. The previous view container signals will
     * be automatically disconnected.
     */
    void setViewContainer(DolphinViewContainer* container);

signals:
    /**
     * See DolphinViewContainer::showFilterBarChanged()
     */
    void showFilterBarChanged(bool shown);

    /**
     * See DolphinViewContainer::writeStateChanged()
     */
    void writeStateChanged(bool isFolderWritable);

    /**
     * See DolphinView::requestItemInfo()
     */
    void requestItemInfo(const KFileItem& item);

    /**
     * See DolphinView::selectionChanged()
     */
    void selectionChanged(const KFileItemList& selection);

    /**
     * See DolphinView::requestContextMenu()
     */
    void requestContextMenu(const QPoint& pos,
                            const KFileItem& item,
                            const KUrl& url,
                            const QList<QAction*>& customActions);

    /**
     * See DolphinView::directoryLoadingStarted()
     */
    void directoryLoadingStarted();

    /**
     * See DolphinView::directoryLoadingCompleted()
     */
    void directoryLoadingCompleted();

    /**
     * See DolphinView::goBackRequested()
     */
    void goBackRequested();

    /**
     * See DolphinView::goForwardRequested()
     */
    void goForwardRequested();

    /**
     * See KUrlNavigator::urlChanged()
     */
    void urlChanged(const KUrl& url);

    /**
     * See KUrlNavigator::historyChanged()
     */
    void historyChanged();

    /**
     * See KUrlNavigator::editableStateChanged()
     */
    void editableStateChanged(bool editable);

    /**
     * See DolphinView::tabRequested() and KUrlNavigator::tabRequested()
     */
    void tabRequested(const KUrl& url);

private:
    DolphinViewContainer* m_viewContainer;
};

#endif