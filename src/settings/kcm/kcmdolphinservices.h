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

#ifndef KCMDOLPHINSERVICES_H
#define KCMDOLPHINSERVICES_H

#include <kcmodule.h>

class ServicesSettingsPage;

/**
 * @brief Allow to configure the Dolphin services.
 */
class DolphinServicesConfigModule : public KCModule
{
    Q_OBJECT

public:
    DolphinServicesConfigModule(QWidget* parent, const QVariantList& args);
    virtual ~DolphinServicesConfigModule();

    virtual void save();
    virtual void defaults();

private:
    ServicesSettingsPage* m_services;
};

#endif
