/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef KCMDOLPHIN_H
#define KCMDOLPHIN_H

#include <kcmodule.h>

class ViewSettingsPageBase;

/**
 * @brief Allow to configure the Dolphin views.
 */
class DolphinConfigModule : public KCModule
{
    Q_OBJECT

public:
    DolphinConfigModule(QWidget* parent, const QVariantList& args);
    virtual ~DolphinConfigModule();

    virtual void save();
    virtual void defaults();

private:
    void reparseConfiguration();

private:
    QList<ViewSettingsPageBase*> m_pages;
};

#endif
