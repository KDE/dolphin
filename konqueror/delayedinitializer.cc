
#include "delayedinitializer.h"
#include <qtimer.h>

DelayedInitializer::DelayedInitializer( int eventType, QObject *parent, const char *name )
    : QObject( parent, name ), m_eventType( eventType ), m_signalEmitted( false )
{
    parent->installEventFilter( this );
}

bool DelayedInitializer::eventFilter( QObject *receiver, QEvent *event )
{
    if ( m_signalEmitted || event->type() != m_eventType )
        return false;

    m_signalEmitted = true;
    receiver->removeEventFilter( this );

    // Move the emitting of the event to the end of the eventQueue
    // so we are absolutely sure the event we get here is handled before
    // the initialize is fired.
    QTimer::singleShot( 0, this, SLOT( slotInitialize() ) );

    return false;
}

void DelayedInitializer::slotInitialize()
{
    emit initialize();
    deleteLater();
}

#include "delayedinitializer.moc"

/* vim: et sw=4
 */
