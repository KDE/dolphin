/****************************************************************************
    Implementation of QXEmbed class

   Copyright (C) 1999-2000 Troll Tech AS

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*****************************************************************************/

#include "knspluginembed.h"

#include <kdebug.h>
#include <klocale.h>

#include <qapplication.h>
#include <qevent.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
// avoid name clashes between X and Qt
const int XFocusOut = FocusOut;
const int XFocusIn = FocusIn;
const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
#undef KeyRelease
#undef KeyPress
#undef FocusOut
#undef FocusIn


#include "kqeventutil.h"
#include "kxeventutil.h"


struct KNSPluginEmbedPrivate
{
public:
  QPoint lastPos;
};

/*!\reimp
 */
bool KNSPluginEmbed::eventFilter( QObject* o, QEvent* e)
{
    QEvent::Type t = e->type();

    if( t != QEvent::MouseMove && t != QEvent::Timer && t <= QEvent::User )
    {
        //kdDebug(6100) << "KNSPluginEmbed::eventFilter, event = " << getQtEventName( e ) << endl;
        if( o == this )
        {
            //kdDebug(6100) << "event is for me:)" << endl;
        }

        switch ( e->type() )
        {

            case QEvent::FocusIn:
                break;

            case QEvent::FocusOut:
                break;

            case QEvent::Leave:
                /* check to see if we are entering the applet somehow... */
                break;

            case QEvent::Enter:
                break;

            case QEvent::WindowActivate:
    	        break;

            case QEvent::WindowDeactivate:
    	        break;

            case QEvent::Move:
               {
                    // if the browser window is moved or the page scrolled,
                    // AWT buttons do not respond anymore (although they are
                    // visually updated!) and subframes of select boxes appear
                    // on completely wrong (old) positions.
                    // This fixes the problem by notifying the applet.
                    QPoint globalPos = mapToGlobal(QPoint(0,0));
                    if (globalPos != d->lastPos) {
                        d->lastPos = globalPos;
                        sendSyntheticConfigureNotifyEvent();
                    }
                }
                break;

            default:
                break;
        }

    }

    return FALSE;
}

/*!
  Constructs a xembed widget.

  The \e parent, \e name and \e f arguments are passed to the QFrame
  constructor.
 */
KNSPluginEmbed::KNSPluginEmbed( QWidget *parent, const char *name, WFlags f )
  : QWidget( parent, name, f )
{
    d = new KNSPluginEmbedPrivate;

    setFocusPolicy( StrongFocus );
    setKeyCompression( FALSE );
    setAcceptDrops( TRUE );

    window = 0;
    // we are interested in SubstructureNotify
    XSelectInput(qt_xdisplay(), winId(),
                 KeyPressMask | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask |
                 KeymapStateMask |
                 ButtonMotionMask |
                 PointerMotionMask | // may need this, too
                 EnterWindowMask | LeaveWindowMask |
                 FocusChangeMask |
                 ExposureMask |
                 StructureNotifyMask |
                 SubstructureRedirectMask |
                 SubstructureNotifyMask
                 );

    topLevelWidget()->installEventFilter( this );
    qApp->installEventFilter( this );

}

/*!
  Destructor. Cleans up the focus if necessary.
 */
KNSPluginEmbed::~KNSPluginEmbed()
{
    if ( window != 0 )
    {
        XUnmapWindow( qt_xdisplay(), window );
        QApplication::flushX();
    }

    delete d;
}

/*!\reimp
 */
void KNSPluginEmbed::resizeEvent( QResizeEvent* e )
{
//    kdDebug(6100) << "KNSPluginEmbed::resizeEvent" << endl;
    QWidget::resizeEvent( e );

    if ( window != 0 )
        XResizeWindow( qt_xdisplay(), window, e->size().width(), e->size().height() );
}

bool  KNSPluginEmbed::event( QEvent* e)
{
//    kdDebug(6100) << "KNSPluginEmbed::event, event type = " << getQtEventName( e ) << endl;
    switch( e->type() )
    {
        case QEvent::ShowWindowRequest:
        case QEvent::WindowActivate:
            XMapRaised( qt_xdisplay(), window );
            QApplication::syncX();
            break;
        default:
            break;
    }
    return QWidget::event( e );
}

/*!\reimp
 */
void KNSPluginEmbed::focusInEvent( QFocusEvent* )
{
//    kdDebug(6100) << "KNSPluginEmbed::focusInEvent" << endl;

    if ( !window )
        return;

    XEvent ev;
    memset( &ev, 0, sizeof( ev ) );
    ev.xfocus.type = XFocusIn;
    ev.xfocus.window = window;

    XSendEvent( qt_xdisplay(), window, true, NoEventMask, &ev );
}

/*!\reimp
 */
void KNSPluginEmbed::focusOutEvent( QFocusEvent* )
{
//    kdDebug(6100) << "KNSPluginEmbed::focusOutEvent" << endl;

    if ( !window )
        return;

    XEvent ev;
    memset( &ev, 0, sizeof( ev ) );
    ev.xfocus.type = XFocusOut;
    ev.xfocus.window = window;
    XSendEvent( qt_xdisplay(), window, true, NoEventMask, &ev );
}


/*!
  Embeds the window with the identifier \a w into this xembed widget.

  This function is useful if the server knows about the client window
  that should be embedded.  Often it is vice versa: the client knows
  about its target embedder. In that case, it is not necessary to call
  embed(). Instead, the client will call the static function
  embedClientIntoWindow().

  \sa embeddedWinId()
 */
void KNSPluginEmbed::embed( WId w )
{
//    kdDebug(6100) << "KNSPluginEmbed::embed" << endl;

    if ( w == 0 )
        return;

    window = w;

    //first withdraw the window
    QApplication::flushX();
    XWithdrawWindow( qt_xdisplay(), window, qt_xscreen() );

    //make sure we will receive destroy notifications for the embedded window.
    XWindowAttributes xwattr;
    XGetWindowAttributes(qt_xdisplay(), winId(), &xwattr);
    XSelectInput(qt_xdisplay(), winId(), SubstructureNotifyMask | xwattr.your_event_mask);

    //now reparent the window to be swallowed by the KNSPluginEmbed widget
    XReparentWindow( qt_xdisplay(), window, winId(), 0, 0 );
    //and add it to the save set in case this window gets destroyed before
    XAddToSaveSet(qt_xdisplay(), window);
    QApplication::syncX();

    //now resize it
    XResizeWindow( qt_xdisplay(), window, width(), height() );
    XMapRaised( qt_xdisplay(), window );

    setFocus();
}

/*!\reimp
 */
bool KNSPluginEmbed::focusNextPrevChild( bool next )
{
    if ( window )
        return FALSE;
    else
        return QWidget::focusNextPrevChild( next );
}

/*!\reimp
 */
bool KNSPluginEmbed::x11Event( XEvent* e)
{
//    kdDebug(6100) << "KNSPluginEmbed::x11Event, event = " << getX11EventName( e )
//        << ", window = " << e->xany.window << endl;

    switch ( e->type )
    {
        case DestroyNotify:
            if ( e->xdestroywindow.window == window )
            {
                window = 0;
            }
            break;
        case ConfigureRequest:
            if (e->xconfigurerequest.window == window)
            {
                    sendSyntheticConfigureNotifyEvent();
            }
            break;
    	case LeaveNotify:
    	    break;
    	case XFocusOut:
    	    break;
        case EnterNotify:
            break;
        case XFocusIn:
            break;

        default:
	        break;
    }

    return false;
}


void KNSPluginEmbed::sendSyntheticConfigureNotifyEvent() {
    QPoint globalPos = mapToGlobal(QPoint(0,0));
    if (window) {
        // kdDebug(6100) << "*************** sendSyntheticConfigureNotify ******************" << endl;
        XConfigureEvent c;
        memset(&c, 0, sizeof(c));
        c.type = ConfigureNotify;
        c.display = qt_xdisplay();
        c.send_event = True;
        c.event = window;
        c.window = winId();
        c.x = globalPos.x();
        c.y = globalPos.y();
        c.width = width();
        c.height = height();
        c.border_width = 0;
        c.above = None;
        c.override_redirect = 0;
        XSendEvent( qt_xdisplay(), c.event, TRUE, StructureNotifyMask, (XEvent*)&c );
    }
}




/*!
  Specifies that this widget can use additional space, and that it can
  survive on less than sizeHint().
*/

QSizePolicy KNSPluginEmbed::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

/*!
  Returns a size sufficient for the embedded window
*/
QSize KNSPluginEmbed::sizeHint() const
{
    return minimumSizeHint();
}

/*!
  Returns the minimum size specified by the embedded window.
*/
QSize KNSPluginEmbed::minimumSizeHint() const
{
    int minw = 0;
    int minh = 0;
    if ( window )
    {
        kdDebug(6100) << "KNSPluginEmbed::minimumSizeHint, getting hints from window" << endl;

        XSizeHints size;
        long msize;
        if( XGetWMNormalHints( qt_xdisplay(), window, &size, &msize ) &&
            ( size.flags & PMinSize) )
        {
            minw = size.min_width;
            minh = size.min_height;
            kdDebug(6100) << "XGetWMNormalHints succeeded, minw = " << minw << ", minh = " << minh << endl;
        }
    }

    return QSize( minw, minh );
}

// for KDE
#include "knspluginembed.moc"
