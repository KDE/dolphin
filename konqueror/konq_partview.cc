
#include "konq_partview.h"

/*
 * Simon's small TODO list :)
 * - emit a fake eventChildGotFocus Event to the Parent Part (only if the
 *   parent part supports the KonqMainView interface) when the attached
 *   part gets the focus. In order to implement this I have to:
 *  1) make the attached part a child part
 *  2) if the child part gets the focus
 *     then do this:
 *     - emit our fake event to the "real" parent
 *     - remove us (KonqPartView) as part parent from our child
 *     - call m_vMainWindow->setActivePart(...)
 *     - re-parent the attached part again
 *     - do something I forgot?
 *  ...tricky, tricky, I wonder whether it will work ;-]
 */

KonqPartView::KonqPartView()
{
  ADD_INTERFACE( "IDL:Konqueror/PartView:1.0" );

  setWidget( this );
  
  m_vPart = 0L;
  m_pFrame = 0L;
}

KonqPartView::~KonqPartView()
{
  cleanUp();
}

void KonqPartView::init()
{
  m_pFrame = new OPFrame( this );
}

void KonqPartView::cleanUp()
{
  if ( m_bIsClean )
    return;
    
  if ( !CORBA::is_nil( m_vPart ) )
    m_vPart = 0L;
    
  if ( m_pFrame )    
  {
    m_pFrame->detach();
    delete m_pFrame;
  }    
    
  KonqBaseView::cleanUp();    
}

bool KonqPartView::mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu )
{
  OpenPartsUI::Menu_var menu = OpenPartsUI::Menu::_duplicate( viewMenu.menu );
  
  if ( !CORBA::is_nil( menu ) )
  {
    menu->insertItem( i18n("Detach part"), this, "detachPart", 0 );
  }
}

void KonqPartView::detachPart()
{
  if ( m_pFrame )
  {
    m_pFrame->detach();
    m_pFrame->hide();
    m_vPart = 0L;
  }    
}

void KonqPartView::setPart( OpenParts::Part_ptr part )
{
  m_vPart = OpenParts::Part::_duplicate( part );
  m_pFrame->attach( m_vPart );
  m_pFrame->show();
}

OpenParts::Part_ptr KonqPartView::part()
{
  return OpenParts::Part::_duplicate( m_vPart );
}

void KonqPartView::resizeEvent( QResizeEvent * )
{
  if ( m_pFrame )
    m_pFrame->setGeometry( 0, 0, width(), height() );
}

#include "konq_partview.moc"