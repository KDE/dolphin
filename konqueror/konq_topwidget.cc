#include <qdir.h>

#include "konq_mainwindow.h"
#include "konq_topwidget.h"

#include <kapp.h>

#include <X11/Xlib.h>

KonqTopWidget* KonqTopWidget::pKonqTopWidget = 0L;
bool KonqTopWidget::s_bGoingDown = false;

KonqTopWidget::KonqTopWidget()
{
  pKonqTopWidget = this;
    
  kapp->setTopWidget( this );

  kapp->enableSessionManagement( TRUE );
  kapp->setWmCommand( "" );
    
  connect( kapp, SIGNAL( saveYourself() ), this, SLOT( slotSave() ) );
  connect( kapp, SIGNAL( shutDown() ), this, SLOT( slotShutDown() ) );

  KonqMainWindow::restoreAllWindows();
}

KonqTopWidget::~KonqTopWidget()
{
}
    
void KonqTopWidget::slotSave()
{
  kdebug(KDEBUG_INFO, 1202, "KonqTopWidget::slotSave()");
  
  KonqMainWindow::saveAllWindows();

  // Make sure slotShutDown() gets called
  XSetIOErrorHandler( XSetIOErrorHandler( 0 ) );
}

void KonqTopWidget::slotShutDown()
{
  s_bGoingDown = true;
}

#include "konq_topwidget.moc"

