/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __kserviceselectdlg_h
#define __kserviceselectdlg_h
#include <kdialog.h>
#include <kservice.h>
#include <klistbox.h>
class KServiceSelectDlg : public KDialog
{
    Q_OBJECT
public:
    /**
     * Create a dialog to select a service (not application) for a given service type.
     *
     * @param serviceType the service type we want to choose a service for.
     * @param value is the initial service to select (not implemented currently)
     * @param parent parent widget
     */
    KServiceSelectDlg( const QString& serviceType, const QString& value = QString(), QWidget *parent = 0L );

    ~KServiceSelectDlg();

    /**
     * @return the chosen service
     */
    KService::Ptr service();
private:
    KListBox * m_listbox;
};

#endif
