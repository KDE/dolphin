#ifndef __konq_browseriface_h__
#define __konq_browseriface_h__

#include <kparts/browserinterface.h>

class KonqView;

class KonqBrowserInterface : public KParts::BrowserInterface
{
    Q_OBJECT
    Q_PROPERTY( uint historyLength READ historyLength );
public:
    KonqBrowserInterface( KonqView *view, const char *name );

    uint historyLength() const;

public slots:
    void goHistory( int );

private:
    KonqView *m_view;
};

#endif
