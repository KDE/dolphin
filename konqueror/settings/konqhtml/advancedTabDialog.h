/*
 *  advancedTabDialog.h
 *
 *  Copyright (c) 2002 Aaron J. Seigo <aseigo@olympusproject.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 */

#ifndef __ADVANCEDTABDIALOG_H
#define __ADVANCEDTABDIALOG_H

#include <kdialog.h>

class advancedTabOptions;

class advancedTabDialog : public KDialog
{
    Q_OBJECT

    public:
        advancedTabDialog(QWidget* parent, KSharedConfig::Ptr config, const char* name);
        ~advancedTabDialog();

    protected Q_SLOTS:
        void load();
        void save();
        void changed();

    private:
        KSharedConfig::Ptr m_pConfig;
        advancedTabOptions* m_advancedWidget;
};

#endif
