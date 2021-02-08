/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLACESITEMSIGNALHANDLER_H
#define PLACESITEMSIGNALHANDLER_H

#include <QObject>

class PlacesItem;

/**
 * @brief Helper class for PlacesItem to be able to listen to signals
 *        and performing a corresponding action.
 *
 * PlacesItem is derived from KStandardItem, which is no QObject-class
 * on purpose. To be able to internally listen to signals and performing a
 * corresponding action, PlacesItemSignalHandler is used.
 *
 * E.g. if the PlacesItem wants to react on accessibility-changes of a storage-access,
 * the signal-handler can be used like this:
 * <code>
 *     QObject::connect(storageAccess, SIGNAL(accessibilityChanged(bool,QString)),
 *                      signalHandler, SLOT(onAccessibilityChanged()));
 * </code>
 *
 * The slot PlacesItemSignalHandler::onAccessibilityChanged() will call
 * the method PlacesItem::onAccessibilityChanged().
 */
class PlacesItemSignalHandler: public QObject
{
    Q_OBJECT

public:
    explicit PlacesItemSignalHandler(PlacesItem* item, QObject* parent = nullptr);
    ~PlacesItemSignalHandler() override;

public Q_SLOTS:
    /**
     * Calls PlacesItem::onAccessibilityChanged()
     */
    void onAccessibilityChanged();

    void onTearDownRequested(const QString& udi);

    void onTrashEmptinessChanged(bool isTrashEmpty);

Q_SIGNALS:
    void tearDownExternallyRequested(const QString& udi);

private:
    PlacesItem* m_item;
};

#endif
