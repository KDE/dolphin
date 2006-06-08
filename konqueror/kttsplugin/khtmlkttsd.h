/***************************************************************************
  Copyright (C) 2002 by George Russell <george.russell@clara.net>
  Copyright (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef KHTMLKTTSD_H
#define KHTMLKTTSD_H

#include <kparts/plugin.h>

class KUrl;
class KInstance;

/**
 * KHTML KParts Plugin
 */
class KHTMLPluginKTTSD : public KParts::Plugin
{
    Q_OBJECT
public:

    /**
     * Construct a new KParts plugin.
     */
    KHTMLPluginKTTSD( QObject* parent, const QStringList& );

    /**
     * Destructor.
     */
    virtual ~KHTMLPluginKTTSD();
public Q_SLOTS:
    void slotReadOut();
};

#endif
