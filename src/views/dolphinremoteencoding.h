/*
 * SPDX-FileCopyrightText: 2009 Rahman Duran <rahman.duran@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINREMOTEENCODING_H
#define DOLPHINREMOTEENCODING_H

#include "dolphin_export.h"

#include <QAction>
#include <QStringList>
#include <QUrl>

class KActionMenu;
class DolphinViewActionHandler;

/**
 * @brief Allows to change character encoding for remote urls like ftp.
 *
 * When browsing remote url, its possible to change encoding from Tools Menu.
 */

class DOLPHIN_EXPORT DolphinRemoteEncoding : public QObject
{
    Q_OBJECT
public:
    DolphinRemoteEncoding(QObject *parent, DolphinViewActionHandler *actionHandler);
    ~DolphinRemoteEncoding() override;

public Q_SLOTS:
    void slotAboutToOpenUrl();
    void slotItemSelected(QAction *action);
    void slotReload();
    void slotDefault();

private Q_SLOTS:
    void slotAboutToShow();

private:
    void updateView();
    void loadSettings();
    void fillMenu();
    void updateMenu();

    KActionMenu *m_menu;
    QStringList m_encodingDescriptions;
    QUrl m_currentURL;
    DolphinViewActionHandler *m_actionHandler;

    bool m_loaded;
    int m_idDefault;
};

#endif
