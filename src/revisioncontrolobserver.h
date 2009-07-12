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

#ifndef REVISIONCONTROLOBSERVER_H
#define REVISIONCONTROLOBSERVER_H

#include <libdolphin_export.h>

#include <QObject>
#include <QString>

class DolphinModel;
class KDirLister;
class QAbstractItemView;
class QTimer;
class RevisionControlPlugin;

/**
 * @brief Observes all revision control plugins.
 *
 * The item view gets updated automatically if the currently shown
 * directory is under revision control.
 *
 * @see RevisionControlPlugin
 */
class LIBDOLPHINPRIVATE_EXPORT RevisionControlObserver : public QObject
{
    Q_OBJECT

public:
    RevisionControlObserver(QAbstractItemView* view);
    virtual ~RevisionControlObserver();

private slots:
    void delayedDirectoryVerification();
    void verifyDirectory();

private:
    void updateItemStates();

private:
    QAbstractItemView* m_view;
    KDirLister* m_dirLister;
    DolphinModel* m_dolphinModel;
    QTimer* m_dirVerificationTimer;
    RevisionControlPlugin* m_plugin;
};

#endif // REVISIONCONTROLOBSERVER_H

