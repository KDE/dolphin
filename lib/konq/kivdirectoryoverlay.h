/*  This file is part of the KDE libraries
    Copyright (C) 2002 Simon MacMullen

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef _KIVDIRECTORYOVERLAY_H_
#define _KIVDIRECTORYOVERLAY_H_

#include <kfileitem.h>
#include <libkonq_export.h>

#include <q3dict.h>
//Added by qt3to4:
#include <QTimerEvent>

class KDirLister;
class KFileIVI;

class LIBKONQ_EXPORT KIVDirectoryOverlay : public QObject
{
    Q_OBJECT
public:
    KIVDirectoryOverlay(KFileIVI* directory);
    virtual ~KIVDirectoryOverlay();
    void start();

Q_SIGNALS:
    void finished();

protected:
    virtual void timerEvent(QTimerEvent *);

private Q_SLOTS:
    void slotCompleted();
    void slotNewItems( const KFileItemList& items );

private:
    KDirLister* m_lister;
    bool m_foundItems;
    bool m_containsFolder;
    Q3Dict<int>* m_popularIcons;
    QString m_bestIcon;
    KFileIVI* m_directory;
};

#endif
