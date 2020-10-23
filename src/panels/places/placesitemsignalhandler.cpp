/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placesitemsignalhandler.h"

#include "placesitem.h"

PlacesItemSignalHandler::PlacesItemSignalHandler(PlacesItem* item,
                                                 QObject* parent) :
    QObject(parent),
    m_item(item)
{
}

PlacesItemSignalHandler::~PlacesItemSignalHandler()
{
}

void PlacesItemSignalHandler::onAccessibilityChanged()
{
    if (m_item) {
        m_item->onAccessibilityChanged();
    }
}

void PlacesItemSignalHandler::onTearDownRequested(const QString& udi)
{
    Q_UNUSED(udi)
    if (m_item) {
        Solid::StorageAccess *tmp = m_item->device().as<Solid::StorageAccess>();
        if (tmp) {
            Q_EMIT tearDownExternallyRequested(tmp->filePath());
        }
    }
}

void PlacesItemSignalHandler::onTrashEmptinessChanged(bool isTrashEmpty)
{
    if (m_item) {
        m_item->setIcon(isTrashEmpty ? QStringLiteral("user-trash") : QStringLiteral("user-trash-full"));
    }
}

