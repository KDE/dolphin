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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <qstring.h>
#include <qstringlist.h>

#include "kxeventutil.h"

QString KXEventUtil::getXAnyEventInfo(XEvent *xevent) {
    XAnyEvent *e = &xevent->xany;
    QString winname("window");
    switch (e->type) {
        case  GraphicsExpose:
        case  NoExpose:
            winname="drawable";
            break;
        case  CreateNotify:
        case  ConfigureRequest:
            winname="parent";
            break;
        case  DestroyNotify:
        case  ConfigureNotify:
        case  MapNotify:
        case  ReparentNotify:
        case  UnmapNotify:
            winname="event";
        default:
            break;
    } 
    QString s("serial=%1 send_event=%2 display=0x%3 %4=%5");
    return 
     s.arg(e->serial)
     .arg(e->send_event)
     .arg((long)e->display, 0, 16)
     .arg(winname)
     .arg(e->window);
}
QString KXEventUtil::getXButtonEventInfo(XEvent *xevent) {
    XButtonEvent *e = &xevent->xbutton; 
    QString s("root=%1 subwindow=%2 time=%3 x=%4 y=%5 x_root=%6 y_root=%7 state=%8 button=%9");
    QString t(" same_screen=%1");
    return 
     s.arg(e->root)
     .arg(e->subwindow)
     .arg(e->time)
     .arg(e->x)
     .arg(e->y)
     .arg(e->x_root)
     .arg(e->y_root)
     .arg(e->state)
     .arg(e->button)
     +t.arg(e->same_screen);
}

QString KXEventUtil::getXKeyEventInfo(XEvent *xevent) {
    XKeyEvent *e = &xevent->xkey; 
    QString s("root=%1 subwindow=%2 time=%3 x=%4 y=%5 x_root=%6 y_root=%7 state=%8 keycode=%9");
    QString t(" same_screen=%1");
    return 
     s.arg(e->root)
     .arg(e->subwindow)
     .arg(e->time)
     .arg(e->x)
     .arg(e->y)
     .arg(e->x_root)
     .arg(e->y_root)
     .arg(e->state)
     .arg(e->keycode)
     +t.arg(e->same_screen);
}

QString KXEventUtil::getXMotionEventInfo(XEvent *xevent) {
    XMotionEvent *e = &xevent->xmotion; 
    QString s("root=%1 subwindow=%2 time=%3 x=%4 y=%5 x_root=%6 y_root=%7 state=%8 is_hint=%9");
    QString t(" same_screen=%1");
    return 
     s.arg(e->root)
     .arg(e->subwindow)
     .arg(e->time)
     .arg(e->x)
     .arg(e->y)
     .arg(e->x_root)
     .arg(e->y_root)
     .arg(e->state)
     .arg(e->is_hint)
     +t.arg(e->same_screen);
}
QString KXEventUtil::getXCrossingEventInfo(XEvent *xevent) {
    XCrossingEvent *e = &xevent->xcrossing;
    QString ms, ds;
    switch (e->mode) {
        case NotifyNormal: ms = "NotifyNormal"; break;
        case NotifyGrab: ms = "NotifyGrab"; break;
        case NotifyUngrab: ms = "NotifyUngrab"; break;
        default: ms="?";
    }
    switch (e->detail) {
        case NotifyAncestor: ds = "NotifyAncestor"; break;
        case NotifyVirtual: ds = "NotifyVirtual"; break;
        case NotifyInferior: ds = "NotifyInferior"; break;
        case NotifyNonlinear: ds = "NotifyNonlinear"; break;
        case NotifyNonlinearVirtual: ds = "NotifyNonlinearVirtual"; break;
        default: ds="?";
    }
          
    QString s("root=%1 subwindow=%2 time=%3 x=%4 y=%5 x_root=%6 y_root=%7 mode=%8=%9 ");
    QString t("detail=%1=%2 same_screen=%3 focus=%4 state=%5");
    return 
     s.arg(e->root)
     .arg(e->subwindow)
     .arg(e->time)
     .arg(e->x)
     .arg(e->y)
     .arg(e->x_root)
     .arg(e->y_root)
     .arg(e->mode).arg(ms)
     +
     t.arg(e->detail).arg(ds)
     .arg(e->same_screen)
     .arg(e->focus)
     .arg(e->state);
}
QString KXEventUtil::getXFocusChangeEventInfo(XEvent *xevent) {
    XFocusChangeEvent *e = &xevent->xfocus; 
    QString s("mode=%1 detail=%2");
    return 
     s.arg(e->mode)
     .arg(e->detail);
}
QString KXEventUtil::getXExposeEventInfo(XEvent *xevent) {
    XExposeEvent *e = &xevent->xexpose; 
    QString s("x=%1 y=%2 width=%3 height=%4 count=%5");
    return
     s.arg(e->x)
     .arg(e->y)
     .arg(e->width)
     .arg(e->height)
     .arg(e->count);
}



QString KXEventUtil::getXGraphicsExposeEventInfo(XEvent *xevent) {
    XGraphicsExposeEvent *e = &xevent->xgraphicsexpose; 
    QString s("x=%1 y=%2 width=%3 height=%4 count=%5 major_code=%6 minor_code=%7");
    return 
     s.arg(e->x)
     .arg(e->y)
     .arg(e->width)
     .arg(e->height)
     .arg(e->count)
     .arg(e->major_code)
     .arg(e->minor_code);
}
QString KXEventUtil::getXNoExposeEventInfo(XEvent *xevent) {
    XNoExposeEvent *e = &xevent->xnoexpose; 
    QString s("major_code=%1 minor_code=%2");
    return 
     s.arg(e->major_code)
     .arg(e->minor_code);
}


QString KXEventUtil::getXCreateWindowEventInfo(XEvent *xevent) {
    XCreateWindowEvent *e = &xevent->xcreatewindow; 
    QString s("window=%1 x=%2 y=%3 width=%4 height=%5 border_width=%6 override_redirect=%7");
    return 
     s.arg(e->window)
     .arg(e->x)
     .arg(e->y)
     .arg(e->width)
     .arg(e->height)
     .arg(e->border_width)
     .arg(e->override_redirect);
}

QString KXEventUtil::getXDestroyWindowEventInfo(XEvent *xevent) {
    XDestroyWindowEvent *e = &xevent->xdestroywindow; 
    QString s("window=%1");
    return 
     s.arg(e->window);
}
QString KXEventUtil::getXMapEventInfo(XEvent *xevent) {
    XMapEvent *e = &xevent->xmap; 
    QString s("window=%1 override_redirect=%2");
    return 
     s.arg(e->window)
     .arg(e->override_redirect);
}
QString KXEventUtil::getXMappingEventInfo(XEvent *xevent) {
    XMappingEvent *e = &xevent->xmapping; 
    QString s("request=%1 first_keycode=%2 count=%3");
    return 
     s.arg(e->request)
     .arg(e->first_keycode)
     .arg(e->count);
}
QString KXEventUtil::getXReparentEventInfo(XEvent *xevent) {
    XReparentEvent *e = &xevent->xreparent; 
    QString s("window=%1 parent=%2 x=%3 y=%4");
    return 
     s.arg(e->window)
     .arg(e->parent)
     .arg(e->x)
     .arg(e->y);
}
QString KXEventUtil::getXUnmapEventInfo(XEvent *xevent) {
    XUnmapEvent *e = &xevent->xunmap; 
    QString s("window=%1 from_configure=%2");
    return 
     s.arg(e->window)
     .arg(e->from_configure);
}

QString KXEventUtil::getXConfigureEventInfo(XEvent *xevent) {
    XConfigureEvent *e = &xevent->xconfigure;
    QString s("window=%1 x=%2 y=%2 width=%3 height=%4 border_width=%5 above=%6 override_redirect=%7");
    return 
        s.arg(e->window)
        .arg(e->x).arg(e->y)
        .arg(e->width).arg(e->height)
        .arg(e->border_width)
        .arg(e->above)
        .arg(e->override_redirect);
}

QString KXEventUtil::getXConfigureRequestEventInfo(XEvent *xevent) {
    XConfigureRequestEvent *e = &xevent->xconfigurerequest;
    unsigned vm = e->value_mask;
    QStringList vml;
    if (vm & CWX)           vml.append("CWX");
    if (vm & CWY)           vml.append("CWY");
    if (vm & CWWidth)       vml.append("CWWidth");
    if (vm & CWHeight)      vml.append("CWHeight");
    if (vm & CWBorderWidth) vml.append("CWBorderWidth");
    if (vm & CWSibling)     vml.append("CWSibling");
    if (vm & CWStackMode)   vml.append("CWStackMode");
    QString vms = vml.join("|");
    QString s("window=%1 x=%2 y=%2 width=%3 height=%4 border_width=%5 above=%6 detail=%7 value_mask=0x%8=%9");
    return 
        s.arg(e->window)
        .arg(e->x).arg(e->y)
        .arg(e->width).arg(e->height)
        .arg(e->border_width)
        .arg(e->above)
        .arg(e->detail)
        .arg(e->value_mask, 0, 16)
        .arg(vms);
}
QString KXEventUtil::getX11EventInfo( XEvent* e )
{
    QString anyInfo = getXAnyEventInfo(e);
    QString info = "";
    QString s;
    switch( e->type )
    {
        case KeyPress:
            s = "KeyPress";
            info = getXKeyEventInfo(e);
            break;
        case KeyRelease:
            s = "KeyRelease";
            info = getXKeyEventInfo(e);
            break;
        case ButtonPress:
            s = "ButtonPress";
            info = getXButtonEventInfo(e);
            break;
        case ButtonRelease:
            s = "ButtonRelease";
            info = getXButtonEventInfo(e);
            break;
        case MotionNotify:
            s = "MotionNotify";
            info = getXMotionEventInfo(e);
            break;
        case EnterNotify:
            s = "EnterNotify";
            info = getXCrossingEventInfo(e);
            break;
        case LeaveNotify:
            s = "LeaveNotify";
            info = getXCrossingEventInfo(e);
            break;
        case FocusIn:
            s = "FocusIn";
            info = getXFocusChangeEventInfo(e);
            break;
        case FocusOut:
            s = "FocusOut";
            info = getXFocusChangeEventInfo(e);
            break;
        case KeymapNotify:
            s = "KeymapNotify";
            break;
        case Expose:
            s = "Expose";
            info = getXExposeEventInfo(e);
            break;
        case GraphicsExpose:
            s = "GraphicsExpose";
            info = getXGraphicsExposeEventInfo(e);
            break;
        case NoExpose:
            info = getXNoExposeEventInfo(e);
            s = "NoExpose";
            break;
        case VisibilityNotify:
            s = "VisibilityNotify";
            break;
        case CreateNotify:
            s = "CreateNotify";
            info = getXCreateWindowEventInfo(e);
            break;
        case DestroyNotify:
            s = "DestroyNotify";
            info = getXDestroyWindowEventInfo(e);
            break;
        case UnmapNotify:
            s = "UnmapNotify";
            info = getXUnmapEventInfo(e);
            break;
        case MapNotify:
            s = "MapNotify";
            info = getXMapEventInfo(e);
            break;
        case MapRequest:
            s = "MapRequest";
            break;
        case ReparentNotify:
            s = "ReparentNotify";
            info = getXReparentEventInfo(e);
            break;
        case ConfigureNotify:
            s = "ConfigureNotify";
            info = getXConfigureEventInfo(e);
            break;
        case ConfigureRequest:
            s = "ConfigureRequest";
            info = getXConfigureRequestEventInfo(e);
            break;
        case GravityNotify:
            s = "GravityNotify";
            break;
        case ResizeRequest:
            s = "ResizeRequest";
            break;
        case CirculateNotify:
            s = "CirculateNofify";
            break;
        case CirculateRequest:
            s = "CirculateRequest";
            break;
        case PropertyNotify:
            s = "PropertyNotify";
            break;
        case SelectionClear:
            s = "SelectionClear";
            break;
        case SelectionRequest:
            s = "SelectionRequest";
            break;
        case SelectionNotify:
            s = "SelectionNotify";
            break;
        case ColormapNotify:
            s = "ColormapNotify";
            break;
        case ClientMessage:
            s = "ClientMessage";
            break;
        case MappingNotify:
            s = "MappingNotify";
            info = getXMappingEventInfo(e);
            break;
        case LASTEvent:
            s = "LASTEvent";
            break;

        default:
            s = "Undefined";
            break;
    }

    return s + " " + anyInfo + " " + info;
}
