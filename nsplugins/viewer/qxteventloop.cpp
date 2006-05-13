/****************************************************************************
** Implementation of QWidget class
**
** Created : 931031
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the Xt extension of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/
#include <QApplication>
#include "qxteventloop.h"


#include <qwidgetintdict.h>
//Added by qt3to4:
#include <QEventLoop>
#include <Q3MemArray>

// resolve the conflict between X11's FocusIn and QEvent::FocusIn
const int XFocusOut = FocusOut;
const int XFocusIn = FocusIn;
#undef FocusOut
#undef FocusIn

const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease

Boolean qmotif_event_dispatcher( XEvent *event );

class QXtEventLoopPrivate
{
public:
    QXtEventLoopPrivate();

    void hookMeUp();
    void unhook();

    XtAppContext appContext, ownContext;
    Q3MemArray<XtEventDispatchProc> dispatchers;
    QWidgetIntDict mapper;

    Q3IntDict<QSocketNotifier> socknotDict;
    bool activate_timers;
    int timerid;

    // arguments for Xt display initialization
    const char* applicationClass;
    XrmOptionDescRec* options;
    int numOptions;
};
static QXtEventLoopPrivate *static_d = 0;
static XEvent* last_xevent = 0;


/*! \internal
  Redeliver the given XEvent to Xt.

  Rationale: An XEvent handled by Qt does not go through the Xt event
  handlers, and the internal state of Xt/Motif widgets will not be
  updated.  This function should only be used if an event delivered by
  Qt to a QWidget needs to be sent to an Xt/Motif widget.
*/
bool QXtEventLoop::redeliverEvent( XEvent *event )
{
    // redeliver the event to Xt, NOT through Qt
    if ( static_d->dispatchers[ event->type ]( event ) )
	return TRUE;
    return FALSE;
}


/*!\internal
 */
XEvent* QXtEventLoop::lastEvent()
{
    return last_xevent;
}


QXtEventLoopPrivate::QXtEventLoopPrivate()
    : appContext(NULL), ownContext(NULL),
      activate_timers(FALSE), timerid(-1)
{
}

void QXtEventLoopPrivate::hookMeUp()
{
    // worker to plug Qt into Xt (event dispatchers)
    // and Xt into Qt (QXtEventLoopEventLoop)

    // ### TODO extensions?
    dispatchers.resize( LASTEvent );
    dispatchers.fill( 0 );
    int et;
    for ( et = 2; et < LASTEvent; et++ )
	dispatchers[ et ] =
	    XtSetEventDispatcher( QPaintDevice::x11AppDisplay(),
				  et, ::qmotif_event_dispatcher );
}

void QXtEventLoopPrivate::unhook()
{
    // unhook Qt from Xt (event dispatchers)
    // unhook Xt from Qt? (QXtEventLoopEventLoop)

    // ### TODO extensions?
    int et;
    for ( et = 2; et < LASTEvent; et++ )
	(void) XtSetEventDispatcher( QPaintDevice::x11AppDisplay(),
				     et, dispatchers[ et ] );
    dispatchers.resize( 0 );

    /*
      We cannot destroy the app context here because it closes the X
      display, something QApplication does as well a bit later.
      if ( ownContext )
          XtDestroyApplicationContext( ownContext );
     */
    appContext = ownContext = 0;
}

extern bool qt_try_modal( QWidget *, XEvent * ); // defined in qapplication_x11.cpp
Boolean qmotif_event_dispatcher( XEvent *event )
{
    QApplication::sendPostedEvents();

    QWidgetIntDict *mapper = &static_d->mapper;
    QWidget* qMotif = mapper->find( event->xany.window );
    if ( !qMotif && QWidget::find( event->xany.window) == 0 ) {
	// event is not for Qt, try Xt
	Display* dpy = QPaintDevice::x11AppDisplay();
	Widget w = XtWindowToWidget( dpy, event->xany.window );
	while ( w && ! ( qMotif = mapper->find( XtWindow( w ) ) ) ) {
	    if ( XtIsShell( w ) ) {
		break;
	    }
	    w = XtParent( w );
	}

	if ( qMotif &&
	     ( event->type == XKeyPress || event->type == XKeyRelease ) )  {
	    // remap key events
	    event->xany.window = qMotif->winId();
	}
    }

    last_xevent = event;
    bool delivered = ( qApp->x11ProcessEvent( event ) != -1 );
    last_xevent = 0;
    if ( qMotif ) {
	switch ( event->type ) {
	case EnterNotify:
	case LeaveNotify:
	    event->xcrossing.focus = False;
	    delivered = FALSE;
	    break;
	case XKeyPress:
	case XKeyRelease:
	    delivered = TRUE;
	    break;
	case XFocusIn:
	case XFocusOut:
	    delivered = FALSE;
	    break;
	default:
	    delivered = FALSE;
	    break;
	}
    }

    if ( delivered )
	return True;


    if ( QApplication::activePopupWidget() )
	// we get all events through the popup grabs.  discard the event
	return True;

    if ( qMotif && QApplication::activeModalWidget() ) {
	if ( !qt_try_modal(qMotif, event) )
	    return True;

    }

    if ( static_d->dispatchers[ event->type ]( event ) )
	// Xt handled the event.
	return True;

    return False;
}



/*!
    \class QXtEventLoop
    \brief The QXtEventLoop class is the core behind the Motif Extension.

    \extension Motif

    QXtEventLoop only provides a few public functions, but is the brains
    behind the integration.  QXtEventLoop is responsible for initializing
    the Xt toolkit and the Xt application context.  It does not open a
    connection to the X server, this is done by using QApplication.

    The only member function in QXtEventLoop that depends on an X server
    connection is QXtEventLoop::initialize(). QXtEventLoop must be created before
    QApplication.

    Example usage of QXtEventLoop and QApplication:

    \code
    static char *resources[] = {
	...
    };

    int main(int argc, char **argv)
    {
	QXtEventLoop integrator( "AppClass" );
	XtAppSetFallbackResources( integrator.applicationContext(),
				   resources );
	QApplication app( argc, argv );

	...

	return app.exec();
    }
    \endcode
*/

/*!
  Creates QXtEventLoop, which allows Qt and Xt/Motif integration.

  If \a context is NULL, QXtEventLoop creates a default application context
  itself. The context is accessible through applicationContext().

  All arguments passed to this function (\a applicationClass, \a
  options and \a numOptions) are used to call XtDisplayInitialize()
  after QApplication has been constructed.
*/



QXtEventLoop::QXtEventLoop( const char *applicationClass, XtAppContext context, XrmOptionDescRec *options , int numOptions)
{
#if defined(QT_CHECK_STATE)
    if ( static_d )
	qWarning( "QXtEventLoop: should only have one QXtEventLoop instance!" );
#endif

    d = static_d = new QXtEventLoopPrivate;
    XtToolkitInitialize();
    if ( context )
	d->appContext = context;
    else
	d->ownContext = d->appContext = XtCreateApplicationContext();

    d->applicationClass = applicationClass;
    d->options = options;
    d->numOptions = numOptions;
}


/*!
  Destroys QXtEventLoop.
*/
QXtEventLoop::~QXtEventLoop()
{
  //   d->unhook();
    delete d;
}

/*!
  Returns the application context.
*/
XtAppContext QXtEventLoop::applicationContext() const
{
    return d->appContext;
}


void QXtEventLoop::appStartingUp()
{
    int argc = qApp->argc();
    XtDisplayInitialize( d->appContext,
			 QPaintDevice::x11AppDisplay(),
			 qApp->name(),
			 d->applicationClass,
			 d->options,
			 d->numOptions,
			 &argc,
			 qApp->argv() );
    d->hookMeUp();
}

void QXtEventLoop::appClosingDown()
{
    d->unhook();
}


/*!\internal
 */
void QXtEventLoop::registerWidget( QWidget* w )
{
    if ( !static_d )
	return;
    static_d->mapper.insert( w->winId(), w );
}


/*!\internal
 */
void QXtEventLoop::unregisterWidget( QWidget* w )
{
    if ( !static_d )
	return;
    static_d->mapper.remove( w->winId() );
}


/*! \internal
 */
void qmotif_socknot_handler( XtPointer pointer, int *, XtInputId *id )
{
    QXtEventLoop *eventloop = (QXtEventLoop *) pointer;
    QSocketNotifier *socknot = static_d->socknotDict.find( *id );
    if ( ! socknot ) // this shouldn't happen
	return;
    eventloop->setSocketNotifierPending( socknot );
}

/*! \reimp
 */
void QXtEventLoop::registerSocketNotifier( QSocketNotifier *notifier )
{
    XtInputMask mask;
    switch ( notifier->type() ) {
    case QSocketNotifier::Read:
	mask = XtInputReadMask;
	break;

    case QSocketNotifier::Write:
	mask = XtInputWriteMask;
	break;

    case QSocketNotifier::Exception:
	mask = XtInputExceptMask;
	break;

    default:
	qWarning( "QXtEventLoopEventLoop: socket notifier has invalid type" );
	return;
    }

    XtInputId id = XtAppAddInput( d->appContext,
				  notifier->socket(), (XtPointer) mask,
				  qmotif_socknot_handler, this );
    d->socknotDict.insert( id, notifier );

    QEventLoop::registerSocketNotifier( notifier );
}

/*! \reimp
 */
void QXtEventLoop::unregisterSocketNotifier( QSocketNotifier *notifier )
{
    Q3IntDictIterator<QSocketNotifier> it( d->socknotDict );
    while ( it.current() && notifier != it.current() )
	++it;
    if ( ! it.current() ) {
	// this shouldn't happen
	qWarning( "QXtEventLoopEventLoop: failed to unregister socket notifier" );
	return;
    }

    XtRemoveInput( it.currentKey() );
    d->socknotDict.remove( it.currentKey() );

    QEventLoop::unregisterSocketNotifier( notifier );
}

/*! \internal
 */
void qmotif_timeout_handler( XtPointer, XtIntervalId * )
{
    static_d->activate_timers = TRUE;
    static_d->timerid = -1;
}

/*! \reimp
 */
bool QXtEventLoop::processEvents( ProcessEventsFlags flags )
{
    // Qt uses posted events to do lots of delayed operations, like repaints... these
    // need to be delivered before we go to sleep
    QApplication::sendPostedEvents();

    // make sure we fire off Qt's timers
    int ttw = timeToWait();
    if ( d->timerid != -1 ) {
	XtRemoveTimeOut( d->timerid );
    }
    d->timerid = -1;
    if ( ttw != -1 ) {
	d->timerid =
	    XtAppAddTimeOut( d->appContext, ttw,
			     qmotif_timeout_handler, 0 );
    }

    // get the pending event mask from Xt and process the next event
    XtInputMask pendingmask = XtAppPending( d->appContext );
    XtInputMask mask = pendingmask;
    if ( pendingmask & XtIMTimer ) {
	mask &= ~XtIMTimer;
	// zero timers will starve the Xt X event dispatcher... so process
	// something *instead* of a timer first...
	if ( mask != 0 )
	    XtAppProcessEvent( d->appContext, mask );
	// and process a timer afterwards
	mask = pendingmask & XtIMTimer;
    }

    if ( ( flags & WaitForMore ) )
	XtAppProcessEvent( d->appContext, XtIMAll );
    else
	XtAppProcessEvent( d->appContext, mask );

    int nevents = 0;
    if ( ! ( flags & ExcludeSocketNotifiers ) )
	nevents += activateSocketNotifiers();

    if ( d->activate_timers ) {
	nevents += activateTimers();
    }
    d->activate_timers = FALSE;

    return ( (flags & WaitForMore) || ( pendingmask != 0 ) || nevents > 0 );
}

#include "qxteventloop.moc"


