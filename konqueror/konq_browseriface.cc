
#include "konq_browseriface.h"
#include "konq_view.h"

KonqBrowserInterface::KonqBrowserInterface( KonqView *view, const char *name )
    : KParts::BrowserInterface( view, name )
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

