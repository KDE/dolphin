/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef kshellcmdplugin_h
#define kshellcmdplugin_h

#include <kparts/plugin.h>
#include <klibloader.h>

class KShellCmdPlugin : public KParts::Plugin
{
    Q_OBJECT
public:
    KShellCmdPlugin( QObject* parent, const char* name );
    ~KShellCmdPlugin() {}

public slots:
    void slotExecuteShellCommand();
};

class KShellCmdPluginFactory : public KLibFactory
{
    Q_OBJECT
public:
    KShellCmdPluginFactory( QObject* parent = 0, const char* name = 0 );
    ~KShellCmdPluginFactory();

    virtual QObject* createObject( QObject* parent = 0, const char* pname = 0, const char* name = "QObject", const QStringList &args = QStringList() );

private:
    static KInstance* s_instance;
};

#endif
