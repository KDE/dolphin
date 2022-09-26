/*
 * SPDX-FileCopyrightText: 2014 Frank Reininghaus <frank78ac@googlemail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SPACEINFOOBSERVER_H
#define SPACEINFOOBSERVER_H

#include <QObject>

class QUrl;
class MountPointObserver;

class SpaceInfoObserver : public QObject
{
    Q_OBJECT

public:
    explicit SpaceInfoObserver(const QUrl& url, QObject* parent = nullptr);
    ~SpaceInfoObserver() override;

    quint64 size() const;
    quint64 available() const;

    void setUrl(const QUrl& url);

public Q_SLOTS:
    void update();

Q_SIGNALS:
    /**
     * This signal is emitted when the size or available space changes.
     */
    void valuesChanged();

private Q_SLOTS:
    void spaceInfoChanged(quint64 size, quint64 available);

private:
    MountPointObserver* m_mountPointObserver;

    bool    m_hasData;
    quint64 m_dataSize;
    quint64 m_dataAvailable;
};

#endif
