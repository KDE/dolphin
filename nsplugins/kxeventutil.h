// -*- c++ -*-
/* This file is part of the KDE project
 *
 * Copyright (C) 2002 Till Krech <till@snafu.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef KXEVENTUTIL_H
#define KXEVENTUTIL_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <qstring.h>

class KXEventUtil {
    public:
        static QString getXEventName(XEvent *e);
        static QString getXAnyEventInfo(XEvent *xevent);
        static QString getXButtonEventInfo(XEvent *xevent);
        static QString getXKeyEventInfo(XEvent *xevent);
        static QString getXMotionEventInfo(XEvent *xevent);
        static QString getXCrossingEventInfo(XEvent *xevent);
        static QString getXFocusChangeEventInfo(XEvent *xevent);
        static QString getXExposeEventInfo(XEvent *xevent);
        static QString getXGraphicsExposeEventInfo(XEvent *xevent);
        static QString getXNoExposeEventInfo(XEvent *xevent);
        static QString getXCreateWindowEventInfo(XEvent *xevent);
        static QString getXDestroyWindowEventInfo(XEvent *xevent);
        static QString getXMapEventInfo(XEvent *xevent);
        static QString getXMappingEventInfo(XEvent *xevent);
        static QString getXReparentEventInfo(XEvent *xevent);
        static QString getXUnmapEventInfo(XEvent *xevent);
        static QString getXConfigureEventInfo(XEvent *xevent);
        static QString getXConfigureRequestEventInfo(XEvent *xevent);
        static QString getX11EventInfo( XEvent* e );
};

#endif
