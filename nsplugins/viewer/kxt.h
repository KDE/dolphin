/***************************************************************************
    kxt.h  -  Xt enabled Qt classes (derived from Qt Extension QXt)
                             -------------------
    begin                : Wed Apr 5 2000
    copyright            : (C) 2000 by Stefan Schimanski
    email                : 1Stein@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef KXT_H
#define KXT_H

#include <kapp.h>
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

    static Widget createToplevelWidget();
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
