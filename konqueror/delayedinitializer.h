#ifndef __delayedinitializer_h__
#define __delayedinitializer_h__

#include <qobject.h>

class DelayedInitializer : public QObject
{
    Q_OBJECT
public:
    DelayedInitializer( int eventType, QObject *parent, const char *name = 0 );

    virtual bool eventFilter( QObject *receiver, QEvent *event );

signals:
    void initialize();

private:
    int m_eventType;
    bool m_signalEmitted;
};

#endif
/* vim: et sw=4
 */
