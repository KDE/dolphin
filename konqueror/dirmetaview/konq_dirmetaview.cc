
#include "konq_dirmetaview.h"
#include "konq_events.h"
#include "konq_metadata.h"

#include <qtextbrowser.h>
#include <qmime.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qimage.h>
#include <qfile.h>

#include <konq_factory.h>

#include <kapp.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kseparator.h>
#include <kxmlgui.h>
#include <konqfileitem.h>
#include <kio/global.h>

#include <assert.h>
#include <unistd.h>

using namespace Konqueror;

extern "C"
{
  void *init_libkonqdirmetaview()
  {
    return new DirDetailViewFactory;
  }
}

DirDetailViewFactory::DirDetailViewFactory( QObject *parent, const char *name )
: KParts::Factory( parent, name )
{
  KonqFactory::instanceRef();
  m_dirMimeType = KMimeType::mimeType( "inode/directory" );
}

DirDetailViewFactory::~DirDetailViewFactory()
{
  KonqFactory::instanceUnref();
}

KParts::Part *DirDetailViewFactory::createPart( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *, const QStringList & )
{
  KParts::Part *part = new DirDetailView( this, parentWidget, widgetName, parent, name );
  emit objectCreated( part );
  return part;
}

DetailWidget::DetailWidget( QWidget *parent, const char *name )
: QWidget( parent, name )
{
//  m_bg = KonqFactory::instance()->iconLoader()->loadIcon( "/home/simon/tt2.png" );
}

DetailWidget::~DetailWidget()
{
}

void DetailWidget::paintEvent( QPaintEvent * )
{
  QPixmap buffer( rect().size() );
  QPainter painter;
  painter.begin( &buffer, this );

  QRect r = rect();

  //  painter.fillRect( r, QBrush( colorGroup().base() ) );
  //  painter.drawPixmap( r.topLeft(), m_bg );
  painter.fillRect( r, QBrush( white ) ); // HACK
  //  painter.drawPixmap( r.topLeft(), m_bg );

  r.moveBy( 5, 5 );
  r.setWidth( r.width() - 5 );
  r.setHeight( r.height() - 5 );

  painter.drawPixmap( r.left(), ( r.bottom() / 2 ) - 32, m_pix );

  r.moveBy( m_pix.width() + 5, 0 );

  int x = r.left();
  int y1 = r.top() + 4;
  int y2 = r.bottom() - 4;

  painter.setPen( colorGroup().mid() );
  painter.drawLine( x, y1, x, y2 );
  x++;
  painter.setPen( colorGroup().midlight() );
  painter.drawLine( x, y1, x, y2 );

  r.moveBy( 7, 0 );

  painter.setPen( colorGroup().text() );

  painter.drawText( r, AlignLeft | AlignVCenter, m_text, -1 );

  painter.end();
  bitBlt( this, 0, 0, &buffer );
}

DirDetailView::DirDetailView( DirDetailViewFactory *factory, QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name )
: KParts::ReadOnlyPart( parent, name )
{
  assert( parent );

  parent->installEventFilter( this );

  m_factory = factory;

  setInstance( KonqFactory::instance(), false );
  //  setXMLFile( "konq_dirdetailview.rc" );

  m_widget = new DetailWidget( parentWidget, widgetName );
  setWidget( m_widget );

  m_widget->setFixedHeight( 67 ); // ### hack
  
  m_metaDataProvider = new KonqMetaDataProvider( KonqFactory::instance(), this, "metaprov" );
}

DirDetailView::~DirDetailView()
{
}

bool DirDetailView::openURL( const KURL &url )
{
  m_metaDataProvider->openDir( url );
 
  bool res = openURL( url, KonqFileItemList() );
  m_url = url;
  return res;
}

bool DirDetailView::openURL( const KURL &url, KonqFileItemList selection )
{
//  kdDebug() <<  "DirDetailView::openURL( " << url.url() << " ) " << endl;

  bool update = false;

  if ( ( selection.count() == 0 && ( m_currentSelection.count() != 0 || m_url != url ) ) ||
       ( selection.count() > 1 ) )
  {
    QPixmap pix = m_factory->dirMimeType()->pixmap( url, KIconLoader::Large );

    m_widget->setPixmap( pix );
    
    QString annotation;
    
    if ( m_metaDataProvider->metaData( url, "inode/directory", "annotation", annotation ) && !annotation.isEmpty() )
      m_widget->setText( annotation );
    else
      m_widget->setText( url.decodedURL() );

    m_currentSelection.clear();
    update = true;
  }
  else if ( selection.count() == 1 && ( m_currentSelection.count() != 1 || selection.first()->url() != m_currentSelection.first() ) )
  {
    KonqFileItem *item = selection.first();

    m_widget->setPixmap( item->pixmap( KIconLoader::Large, true ) );
    
    QString annotation;
    
    if ( m_metaDataProvider->metaData( item->url(), item->mimetype(), "annotation", annotation ) && !annotation.isEmpty() )
      m_widget->setText( annotation );
    else
      m_widget->setText( url.decodedURL() );

    m_currentSelection.clear();
    m_currentSelection.append( item->url() );
    update = true;
  }

  if ( update )
    m_widget->update();

  return true;
}

bool DirDetailView::eventFilter( QObject *, QEvent *event )
{
  if ( !KonqFileSelectionEvent::test( event ) )
    return false;

  KonqFileSelectionEvent *ev = static_cast<KonqFileSelectionEvent *>( event );

  if ( static_cast<QObject *>( ev->part() ) != parent() )
    return false;

  openURL( m_url, ev->selection() );

  return false;
}

#include "konq_dirmetaview.moc"
