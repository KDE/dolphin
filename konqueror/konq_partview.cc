
#include "konq_partview.h"

KonqPartView::KonqPartView()
{
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

#include "konq_partview.moc"