/*

  kxt.h  -  Xt enabled Qt classed (derived from Qt Extension QXt)

  Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

/****************************************************************************
**
** Definition of Qt extension classes for Xt/Motif support.
**
** Created : 980107
**
** Copyright (C) 1992-2000 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing, or see
** http://www.trolltech.com/qpl/ for QPL licensing information.
**
*****************************************************************************/
#ifndef KXT_H
#define KXT_H

#include <qglobal.h>
#if QT_VERSION < 0x030100

#include <kapplication.h>
#include <qwidget.h>
#include <X11/Intrinsic.h>

class KXtApplication : public KApplication {
    Q_OBJECT
    void init();

public:
    KXtApplication(int& argc, char** argv,
	const QCString& rAppName, bool allowStyles=true, bool GUIenabled=true,
	XrmOptionDescRec *options=0, int num_options=0, char** resources=0);
    KXtApplication(Display*, int& argc, char** argv, const QCString& rAppName,
                   bool allowStyles=true, bool GUIenabled=true);
    ~KXtApplication();
};

class KXtWidget : public QWidget {
    Q_OBJECT
    Widget xtw;
    Widget xtparent;
    bool   need_reroot;
    void init(const char* name, WidgetClass widget_class,
		    Widget parent, QWidget* qparent,
		    ArgList args, Cardinal num_args,
		    bool managed);
    friend void qwidget_realize( Widget widget, XtValueMask* mask,
				 XSetWindowAttributes* attributes );

public:
    KXtWidget(const char* name, Widget parent, bool managed=FALSE);
    KXtWidget(const char* name, WidgetClass widget_class,
	      QWidget *parent=0, ArgList args=0, Cardinal num_args=0,
	      bool managed=FALSE);
    ~KXtWidget();

    Widget xtWidget() const { return xtw; }
    bool isActiveWindow() const;
    void setActiveWindow();

protected:
    void moveEvent( QMoveEvent* );
    void resizeEvent( QResizeEvent* );
    bool x11Event( XEvent * );
};

#endif
#endif
