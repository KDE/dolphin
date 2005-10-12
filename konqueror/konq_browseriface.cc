
#include "konq_browseriface.h"
#include "konq_view.h"

KonqBrowserInterface::KonqBrowserInterface( KonqView *view )
    : KParts::BrowserInterface( view )
{
    m_view = view;
}

uint KonqBrowserInterface::historyLength() const
{
    return m_view->historyLength();
}

void KonqBrowserInterface::goHistory( int steps )
{
    m_view->goHistory( steps );
}

#include "konq_browseriface.moc"

