/* This file is part of the KDE libraries
    Copyright (C) 2002 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
//
// File previews configuration
//

#ifndef _PREVIEWS_OPTIONS_H
#define _PREVIEWS_OPTIONS_H

/*

The "Previews" Tab contains :

List of protocols that support listing, with a checkbox for each
Configuration of the maximum image size

*/

#include <qstring.h>

#include <kcmodule.h>

class KConfig;
class QCheckBox;
class QCheckListItem;
class KDoubleNumInput;

class KPreviewOptions : public KCModule
{
    Q_OBJECT
public:
    KPreviewOptions( QWidget *parent = 0L, const char *name = 0L );
    virtual void load();
    virtual void save();
    virtual void defaults();

protected:
    void load(bool useDefaults);

public slots:
    void changed();

private:
    QPtrList<QCheckListItem> m_items;
    KDoubleNumInput *m_maxSize;
    QCheckBox *m_boostSize;
    QCheckBox *m_useFileThumbnails;
};

#endif
