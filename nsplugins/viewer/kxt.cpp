/*

  kxt.cpp  -  Xt enabled Qt classed (derived from Qt Extension QXt)

  Copyright (c) 2000,2001 Stefan Schimanski <1Stein@gmx.de>

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
** Implementation of Qt extension classes for Xt/Motif support.
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

#include <qglobal.h>
#if QT_VERSION < 0x030100

#include <kapplication.h>
#include <qwidget.h>
#include <qobjectlist.h>
#include <qwidgetlist.h>
#include <kdebug.h>
#include <qtimer.h>

#include "kxt.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h> // for XtCreateWindow
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease

extern Atom qt_wm_state;

//#define HAVE_MOTIF
#ifdef HAVE_MOTIF
#include <Xm/Xm.h>
#endif

typedef void (*SameAsXtTimerCallbackProc)(void*,void*);
typedef void (*IntervalSetter)(int);
typedef void (*ForeignEventProc)(XEvent*);

extern XtEventDispatchProc
 qt_np_cascade_event_handler[LASTEvent];      // defined in qnpsupport.cpp
void            qt_reset_color_avail();       // defined in qcolor_x11.cpp
int             qt_activate_timers();         // defined in qapplication_x11.cpp
timeval        *qt_wait_timer();              // defined in qapplication_x11.cpp
void            qt_x11SendPostedEvents();     // defined in qapplication_x11.cpp
int     qt_event_handler( XEvent* event );    // defined in qnpsupport.cpp
extern int      qt_np_count;                  // defined in qnpsupport.cpp
void qt_np_timeout( void* p, void* id );      // defined in qnpsupport.cpp
void qt_np_add_timeoutcb(
        SameAsXtTimerCallbackProc cb );       // defined in qnpsupport.cpp
void qt_np_remove_timeoutcb(
        SameAsXtTimerCallbackProc cb );       // defined in qnpsupport.cpp
void qt_np_add_timer_setter(
        IntervalSetter is );                  // defined in qnpsupport.cpp
void qt_np_remove_timer_setter(
        IntervalSetter is );                  // defined in qnpsupport.cpp
extern XtIntervalId qt_np_timerid;            // defined in qnpsupport.cpp
extern void (*qt_np_leave_cb)
              (XLeaveWindowEvent*);           // defined in qnpsupport.cpp
void qt_np_add_event_proc(
            ForeignEventProc fep );           // defined in qnpsupport.cpp
void qt_np_remove_event_proc(
            ForeignEventProc fep );           // defined in qnpsupport.cpp


typedef struct {
    int empty;
} QWidgetClassPart;

typedef struct _QWidgetClassRec {
    CoreClassPart       core_class;
    QWidgetClassPart    qwidget_class;
} QWidgetClassRec;

//static QWidgetClassRec qwidgetClassRec;

typedef struct {
    /* resources */
    /* (none) */
    /* private state */
    KXtWidget* qxtwidget;
} QWidgetPart;

typedef struct _QWidgetRec {
    CorePart    core;
    QWidgetPart qwidget;
} QWidgetRec;


static
void reparentChildrenOf(QWidget* parent)
{

    if ( !parent->children() )
        return; // nothing to do

    for ( QObjectListIt it( *parent->children() ); it.current(); ++it ) {
        if ( it.current()->isWidgetType() ) {
            QWidget* widget = (QWidget*)it.current();
            XReparentWindow( QX11Info::display(),
                             widget->winId(),
                             parent->winId(),
                             widget->x(),
                             widget->y() );
            if ( widget->isVisible() )
                XMapWindow( QX11Info::display(), widget->winId() );
        }
    }

}

void qwidget_realize(
        Widget                widget,
        XtValueMask*          mask,
        XSetWindowAttributes* attributes
    )
{
    widgetClassRec.core_class.realize(widget, mask, attributes);
    KXtWidget* qxtw = ((QWidgetRec*)widget)->qwidget.qxtwidget;
    if (XtWindow(widget) != qxtw->winId()) {
        qxtw->create(XtWindow(widget), FALSE, FALSE);
        reparentChildrenOf(qxtw);
    }
    qxtw->show();
    XMapWindow( QX11Info::display(), qxtw->winId() );
}

static
QWidgetClassRec qwidgetClassRec = {
  { /* core fields */
    /* superclass               */      (WidgetClass) &widgetClassRec,
    /* class_name               */      (char*)"QWidget",
    /* widget_size              */      sizeof(QWidgetRec),
    /* class_initialize         */      0,
    /* class_part_initialize    */      0,
    /* class_inited             */      FALSE,
    /* initialize               */      0,
    /* initialize_hook          */      0,
    /* realize                  */      qwidget_realize,
    /* actions                  */      0,
    /* num_actions              */      0,
    /* resources                */      0,
    /* num_resources            */      0,
    /* xrm_class                */      NULLQUARK,
    /* compress_motion          */      TRUE,
    /* compress_exposure        */      TRUE,
    /* compress_enterleave      */      TRUE,
    /* visible_interest         */      FALSE,
    /* destroy                  */      0,
    /* resize                   */      XtInheritResize,
    /* expose                   */      XtInheritExpose,
    /* set_values               */      0,
    /* set_values_hook          */      0,
    /* set_values_almost        */      XtInheritSetValuesAlmost,
    /* get_values_hook          */      0,
    /* accept_focus             */      XtInheritAcceptFocus,
    /* version                  */      XtVersion,
    /* callback_private         */      0,
    /* tm_table                 */      XtInheritTranslations,
    /* query_geometry           */      XtInheritQueryGeometry,
    /* display_accelerator      */      XtInheritDisplayAccelerator,
    /* extension                */      0
  },
  { /* qwidget fields */
    /* empty                    */      0
  }
};
static WidgetClass qWidgetClass = (WidgetClass)&qwidgetClassRec;

static bool filters_installed = FALSE;
static KXtApplication* qxtapp = 0;
static XtAppContext appcon;

static
Boolean qt_event_handler_wrapper( XEvent* event )
{
  return (Boolean)qt_event_handler( event );
}

static
void installXtEventFilters()
{
    if (filters_installed) return;
    // Get Xt out of our face - install filter on every event type
    for (int et=2; et < LASTEvent; et++) {
        qt_np_cascade_event_handler[et] = XtSetEventDispatcher(
            QX11Info::display(), et, qt_event_handler_wrapper );
    }
    filters_installed = TRUE;
}

static
void removeXtEventFilters()
{
    if (!filters_installed) return;
    // We aren't needed any more... slink back into the shadows.
    for (int et=2; et < LASTEvent; et++) {
        XtSetEventDispatcher(
            QX11Info::display(), et, qt_np_cascade_event_handler[et] );
    }
    filters_installed = FALSE;
}

// When we are in an event loop of QApplication rather than the browser's
// event loop (eg. for a modal dialog), we still send events to Xt.
static
void np_event_proc( XEvent* e )
{
    Widget xtw = XtWindowToWidget( e->xany.display, e->xany.window );
    if ( xtw && qApp->loopLevel() > 0 ) {
        // Allow Xt to process the event
        qt_np_cascade_event_handler[e->type]( e );
    }
}

static void np_set_timer( int interval )
{
    // Ensure we only have one timeout in progress - QApplication is
    // computing the one amount of time we need to wait.
    if ( qt_np_timerid ) {
        XtRemoveTimeOut( qt_np_timerid );
    }
    qt_np_timerid = XtAppAddTimeOut(appcon, interval,
        (XtTimerCallbackProc)qt_np_timeout, 0);
}

static void np_do_timers( void*, void* )
{
    qt_np_timerid = 0; // It's us, and we just expired, that's why we are here.

    qt_activate_timers();

    timeval *tm = qt_wait_timer();

    if (tm) {
        int interval = QMIN(tm->tv_sec,INT_MAX/1000)*1000 + tm->tv_usec/1000;
        np_set_timer( interval );
    }
    qxtapp->sendPostedEvents();
}

/*!
  \class KXtApplication qxt.h
  \brief Allows mixing of Xt/Motif and Qt widgets.

  \extension Xt/Motif

  The KXtApplication and KXtWidget classes allow old Xt or Motif widgets
  to be used in new Qt applications.  They also allow Qt widgets to
  be used in primarily Xt/Motif applications.  The facility is intended
  to aid migration from Xt/Motif to the more comfortable Qt system.
*/

static bool my_xt;

/*!
  Constructs a QApplication and initializes the Xt toolkit.
  The \a appclass, \a options, \a num_options, and \a resources
  arguments are passed on to XtAppSetFallbackResources and
  XtDisplayInitialize.

  Use this constructor when writing a new Qt application which
  needs to use some existing Xt/Motif widgets.
*/
KXtApplication::KXtApplication(int& argc, char** argv,
        const QCString& rAppName, bool allowStyles, bool GUIenabled,
        XrmOptionDescRec *options, int num_options,
        char** resources)
  : KApplication(argc, argv, rAppName, allowStyles, GUIenabled)
{
    my_xt = TRUE;

    XtToolkitInitialize();
    appcon = XtCreateApplicationContext();
    if (resources) XtAppSetFallbackResources(appcon, (char**)resources);
    XtDisplayInitialize(appcon, QX11Info::display(), name(), rAppName, options,
        num_options, &argc, argv);
    init();
}

/*!
  Constructs a QApplication from the \a display of an already-initialized
  Xt application.

  Use this constructor when introducing Qt widgets into an existing
  Xt/Motif application.
*/
KXtApplication::KXtApplication(Display* dpy, int& argc, char** argv,
        const QCString& rAppName, bool allowStyles, bool GUIenabled)
   : KApplication(dpy, argc, argv, rAppName, allowStyles, GUIenabled)
{
    my_xt = FALSE;
    init();
    appcon = XtDisplayToApplicationContext(dpy);
}

/*!
  Destructs the application.  Does not close the Xt toolkit.
*/
KXtApplication::~KXtApplication()
{
    Q_ASSERT(qxtapp==this);
    removeXtEventFilters();
    qxtapp = 0;

    // the manpage says: "or just exit", that's what we do to avoid
    // double closing of the display
//     if (my_xt) {
//      XtDestroyApplicationContext(appcon);
//      }
}

void KXtApplication::init()
{
    Q_ASSERT(qxtapp==0);
    qxtapp = this;
    installXtEventFilters();
    qt_np_add_timeoutcb(np_do_timers);
    qt_np_add_timer_setter(np_set_timer);
    qt_np_add_event_proc(np_event_proc);
    qt_np_count++;
/*    QTimer *timer = new QTimer( this );
      timer->start(500);*/
}

/*!
  \class KXtWidget qxt.h
  \brief Allows mixing of Xt/Motif and Qt widgets.

  \extension Xt/Motif

  KXtWidget acts as a bridge between Xt and Qt. For utilizing old
  Xt widgets, it can be a QWidget
  based on a Xt widget class. For including Qt widgets in an existing
  Xt/Motif application, it can be a special Xt widget class that is
  a QWidget.  See the constructors for the different behaviors.
*/

void KXtWidget::init(const char* name, WidgetClass widget_class,
                    Widget parent, QWidget* qparent,
                    ArgList args, Cardinal num_args,
                    bool managed)
{
    need_reroot=FALSE;
    xtparent = 0;
    if (parent ) {
        Q_ASSERT(!qparent);
        xtw = XtCreateWidget(name, widget_class, parent, args, num_args);
        if ( widget_class == qWidgetClass )
            ((QWidgetRec*)xtw)->qwidget.qxtwidget = this;
        xtparent = parent;
        if (managed)
            XtManageChild(xtw);
    } else {
        Q_ASSERT(!managed);

        String n, c;
        XtGetApplicationNameAndClass(QX11Info::display(), &n, &c);
        xtw = XtAppCreateShell(n, c, widget_class, QX11Info::display(),
                               args, num_args);
        if ( widget_class == qWidgetClass )
            ((QWidgetRec*)xtw)->qwidget.qxtwidget = this;
    }

    if ( qparent ) {
        XtResizeWidget( xtw, 100, 100, 0 );
        XtSetMappedWhenManaged(xtw, False);
        XtRealizeWidget(xtw);
        XSync(QX11Info::display(), False);    // I want all windows to be created now
        XReparentWindow(QX11Info::display(), XtWindow(xtw), qparent->winId(), x(), y());
        XtSetMappedWhenManaged(xtw, True);
        need_reroot=TRUE;
    }

    Arg reqargs[20];
    Cardinal nargs=0;
    XtSetArg(reqargs[nargs], XtNx, x());        nargs++;
    XtSetArg(reqargs[nargs], XtNy, y());        nargs++;
    XtSetArg(reqargs[nargs], XtNwidth, width());        nargs++;
    XtSetArg(reqargs[nargs], XtNheight, height());      nargs++;
    //XtSetArg(reqargs[nargs], "mappedWhenManaged", False);     nargs++;
    XtSetValues(xtw, reqargs, nargs);

    //#### destroy();   MLK

    if (!parent || XtIsRealized(parent))
        XtRealizeWidget(xtw);
}


/*!
  Constructs a KXtWidget of the special Xt widget class known as
  "QWidget" to the resource manager.

  Use this constructor to utilize Qt widgets in an Xt/Motif
  application.  The KXtWidget is a QWidget, so you can create
  subwidgets, layouts, etc. using Qt functionality.
*/
KXtWidget::KXtWidget(const char* name, Widget parent, bool managed) :
    QWidget( 0, name, WResizeNoErase )
{
    init(name, qWidgetClass, parent, 0, 0, 0, managed);
    Arg reqargs[20];
    Cardinal nargs=0;
    XtSetArg(reqargs[nargs], XtNborderWidth, 0);
    nargs++;
    XtSetValues(xtw, reqargs, nargs);
}

/*!
  Constructs a KXtWidget of the given \a widget_class.

  Use this constructor to utilize Xt or Motif widgets in a Qt
  application.  The KXtWidget looks and behaves
  like the Xt class, but can be used like any QWidget.

  Note that Xt requires that the most toplevel Xt widget is a shell.
  That means, if \a parent is a KXtWidget, the \a widget_class can be
  of any kind. If there isn't a parent or the parent is just a normal
  QWidget, \a widget_class should be something like \c
  topLevelShellWidgetClass.

  If the \a managed parameter is TRUE and \a parent in not NULL,
  XtManageChild it used to manage the child.
*/
KXtWidget::KXtWidget(const char* name, WidgetClass widget_class,
                     QWidget *parent, ArgList args, Cardinal num_args,
                     bool managed) :
    QWidget( parent, name, WResizeNoErase )
{
    if ( !parent )
        init(name, widget_class, 0, 0, args, num_args, managed);
    else if ( parent->inherits("KXtWidget") )
        init(name, widget_class, ( (KXtWidget*)parent)->xtw , 0, args, num_args, managed);
    else
        init(name, widget_class, 0, parent, args, num_args, managed);
    create(XtWindow(xtw), FALSE, FALSE);
}

/*!
  Destructs the KXtWidget.
*/
KXtWidget::~KXtWidget()
{
    // Delete children first, as Xt will destroy their windows
    //
    QObjectList* list = queryList("QWidget", 0, FALSE, FALSE);
    if ( list ) {
        QWidget* c;
        QObjectListIt it( *list );
        while ( (c = (QWidget*)it.current()) ) {
            delete c;
            ++it;
        }
        delete list;
    }

    if ( need_reroot ) {
        hide();
        XReparentWindow(QX11Info::display(), winId(), qApp->desktop()->winId(),
            x(), y());
    }

    XtDestroyWidget(xtw);
    destroy( FALSE, FALSE );
}

/*!
  \fn Widget KXtWidget::xtWidget() const
  Returns the Xt widget equivalent for the Qt widget.
*/



/*!
  Reimplemented to produce the Xt effect of getting focus when the
  mouse enters the widget. <em>This may be changed.</em>
*/
bool KXtWidget::x11Event( XEvent * e )
{
    if ( e->type == EnterNotify ) {
        if  ( xtparent )
            setActiveWindow();
    }
    return QWidget::x11Event( e );
}


/*!
  Implement a degree of focus handling for Xt widgets.
*/
void KXtWidget::setActiveWindow()
{
    if  ( xtparent ) {
        if ( !QWidget::isActiveWindow() && isActiveWindow() ) {
            XFocusChangeEvent e;
            e.type = FocusIn;
            e.window = winId();
            e.mode = NotifyNormal;
            e.detail = NotifyInferior;
            XSendEvent( QX11Info::display(), e.window, TRUE, NoEventMask, (XEvent*)&e );
        }
    } else {
        QWidget::setActiveWindow();
    }
}

/*!
  Different from QWidget::isActiveWindow()
 */
bool KXtWidget::isActiveWindow() const
{
    Window win;
    int revert;
    XGetInputFocus( QX11Info::display(), &win, &revert );

    if ( win == None) return FALSE;

    QWidget *w = find( (WId)win );
    if ( w ) {
        // We know that window
        return w->topLevelWidget() == topLevelWidget();
    } else {
        // Window still may be a parent (if top-level is foreign window)
        Window root, parent;
        Window cursor = winId();
        Window *ch;
        unsigned int nch;
        while ( XQueryTree(QX11Info::display(), cursor, &root, &parent, &ch, &nch) ) {
            if (ch) XFree( (char*)ch);
            if ( parent == win ) return TRUE;
            if ( parent == root ) return FALSE;
            cursor = parent;
        }
        return FALSE;
    }
}

/*!\reimp
 */
void KXtWidget::moveEvent( QMoveEvent* )
{
    if ( xtparent )
        return;
    XConfigureEvent c;
    c.type = ConfigureNotify;
    c.event = winId();
    c.window = winId();
    c.x = x();
    c.y = y();
    c.width = width();
    c.height = height();
    c.border_width = 0;
    XSendEvent( QX11Info::display(), c.event, TRUE, NoEventMask, (XEvent*)&c );
    XtMoveWidget( xtw, x(), y() );
}

/*!\reimp
 */
void KXtWidget::resizeEvent( QResizeEvent* )
{
    if ( xtparent )
        return;
    XtWidgetGeometry preferred;
    (void ) XtQueryGeometry( xtw, 0, &preferred );
    XConfigureEvent c;
    c.type = ConfigureNotify;
    c.event = winId();
    c.window = winId();
    c.x = x();
    c.y = y();
    c.width = width();
    c.height = height();
    c.border_width = 0;
    XSendEvent( QX11Info::display(), c.event, TRUE, NoEventMask, (XEvent*)&c );
    XtResizeWidget( xtw, width(), height(), preferred.border_width );
}

#include "kxt.moc"

#endif
