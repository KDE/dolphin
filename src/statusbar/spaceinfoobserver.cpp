/*
 * SPDX-FileCopyrightText: 2014 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "spaceinfoobserver.h"

#include "mountpointobserver.h"

SpaceInfoObserver::SpaceInfoObserver(const QUrl& url, QObject* parent) :
    QObject(parent),
    m_mountPointObserver(nullptr),
    m_hasData(false),
    m_dataSize(0),
    m_dataAvailable(0)
{
    m_mountPointObserver = MountPointObserver::observerForUrl(url);
    m_mountPointObserver->ref();
    connect(m_mountPointObserver, &MountPointObserver::spaceInfoChanged, this, &SpaceInfoObserver::spaceInfoChanged);
    m_mountPointObserver->update();
}

SpaceInfoObserver::~SpaceInfoObserver()
{
    if (m_mountPointObserver) {
        m_mountPointObserver->deref();
        m_mountPointObserver = nullptr;
    }
}

quint64 SpaceInfoObserver::size() const
{
    return m_dataSize;
}

quint64 SpaceInfoObserver::available() const
{
    return m_dataAvailable;
}

void SpaceInfoObserver::setUrl(const QUrl& url)
{
    MountPointObserver* newObserver = MountPointObserver::observerForUrl(url);
    if (newObserver != m_mountPointObserver) {
        if (m_mountPointObserver) {
            disconnect(m_mountPointObserver, &MountPointObserver::spaceInfoChanged, this, &SpaceInfoObserver::spaceInfoChanged);
            m_mountPointObserver->deref();
            m_mountPointObserver = nullptr;
        }

        m_mountPointObserver = newObserver;
        m_mountPointObserver->ref();
        connect(m_mountPointObserver, &MountPointObserver::spaceInfoChanged, this, &SpaceInfoObserver::spaceInfoChanged);

        // If newObserver is cached it won't call update until the next timer update, 
        // so update the observer now.
        m_mountPointObserver->update();
    }
}

void SpaceInfoObserver::update()
{
    if (m_mountPointObserver) {
        m_mountPointObserver->update();
    }
}

void SpaceInfoObserver::spaceInfoChanged(quint64 size, quint64 available)
{
    // Make sure that the size has actually changed
    if (m_dataSize != size || m_dataAvailable != available || !m_hasData) {
        m_hasData = true;
        m_dataSize = size;
        m_dataAvailable = available;

        Q_EMIT valuesChanged();
    }
}
