
#include "delayedinitializer.h"

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

    emit initialize();

    deleteLater();

    return false;
}

#include "delayedinitializer.moc"

/* vim: et sw=4
 */
