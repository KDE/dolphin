/* This file is part of the KDE libraries
    Copyright (C) 2000 David Faure <faure@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
//
// "Trash" configuration
//

#ifndef _TRASH_OPTIONS_H
#define _TRASH_OPTIONS_H

/*

The "Trash" Tab contains :

Ask confirmation for:
[x] Move To Trash
[x] Delete
[x] Shred

*/

#include <qstring.h>
#include <kcmodule.h>

class KConfig;
class QWidget;
class QRadioButton;
class QCheckBox;

class KTrashOptions : public KCModule
{
        Q_OBJECT
public:
        KTrashOptions(KConfig *config, QString group, QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();
        virtual QString quickHelp() const;

private slots:

        void slotDeleteBehaviourChanged( int );
	void changed();

private:

        KConfig *g_pConfig;
	QString groupname;
        int deleteAction;

        QCheckBox *cbMoveToTrash;
        QCheckBox *cbDelete;
        QCheckBox *cbShred;
};

#endif
