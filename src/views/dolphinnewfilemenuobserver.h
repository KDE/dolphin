/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINNEWFILEMENUOBSERVER_H
#define DOLPHINNEWFILEMENUOBSERVER_H

#include "dolphin_export.h"

#include <QObject>

class DolphinNewFileMenu;

/**
 * @brief Allows to observe new file items that have been created
 *        by a DolphinNewFileMenu instance.
 *
 * As soon as a DolphinNewFileMenu instance created a new item,
 * the observer will emit the signal itemCreated().
 */
class DOLPHIN_EXPORT DolphinNewFileMenuObserver : public QObject
{
    Q_OBJECT

public:
    static DolphinNewFileMenuObserver &instance();
    void attach(const DolphinNewFileMenu *menu);
    void detach(const DolphinNewFileMenu *menu);

Q_SIGNALS:
    void itemCreated(const QUrl &url);
    void errorMessage(const QString &error);

private:
    DolphinNewFileMenuObserver();
    ~DolphinNewFileMenuObserver() override;

    friend class DolphinNewFileMenuObserverSingleton;
};

#endif
