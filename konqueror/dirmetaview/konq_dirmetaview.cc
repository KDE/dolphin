
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
#include <qtimer.h>

#include <konq_factory.h>

#include <kapp.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kseparator.h>
#include <kxmlgui.h>
#include <konqfileitem.h>
#include <kio/global.h>
#include <kaction.h>

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

AnnotationEdit::AnnotationEdit( DetailWidget *parent, const char *name )
: QMultiLineEdit( parent, name )
{
  m_parent = parent;
}

AnnotationEdit::~AnnotationEdit()
{
}

void AnnotationEdit::focusOutEvent( QFocusEvent *e )
{
  QMultiLineEdit::focusOutEvent( e );
  m_parent->editDone();
}

void AnnotationEdit::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Key_Escape )
  {
    m_parent->editDone();
    return;
  }

  QMultiLineEdit::keyPressEvent( e );
}

DetailWidget::DetailWidget( DirDetailView *parent, QWidget *parentWidget, const char *name )
: QWidget( parentWidget, name )
{
//  m_bg = KonqFactory::instance()->iconLoader()->loadIcon( "/home/simon/tt2.png" );
// m_bg = KonqFactory::instance()->iconLoader()->loadIcon( "/home/simon/tron_seamless.png" );
  m_parent = parent;
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

  m_editRect = r;
}

void DetailWidget::mouseReleaseEvent( QMouseEvent *e )
{
  if ( !m_editRect.contains( e->pos() ) )
    return;

  edit();
}

void DetailWidget::edit()
{
  editDone();
  m_edit = new AnnotationEdit( this, "annotedit" );
  m_edit->setGeometry( m_editRect );
  m_edit->show();
  m_edit->setText( m_text );
  m_edit->setFocus();
}

void DetailWidget::editDone()
{
  if ( m_edit )
  {
    m_parent->saveAnnotation( m_edit->text() );
    delete (AnnotationEdit *)m_edit;
  }
}

DirDetailView::DirDetailView( DirDetailViewFactory *factory, QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name )
: KParts::ReadOnlyPart( parent, name )
{
  assert( parent );

  parent->installEventFilter( this );

  m_factory = factory;

  setInstance( KonqFactory::instance(), false );
  setXMLFile( "konq_dirmetaview.rc" );

  m_widget = new DetailWidget( this, parentWidget, widgetName );
  setWidget( m_widget );

  m_widget->setFixedHeight( 67 ); // ### hack

  m_metaDataProvider = new KonqMetaDataProvider( KonqFactory::instance(), this, "metaprov" );

  forceUpdate = false;

  m_paEdit = new KAction( "Write annotation", 0, m_widget, SLOT( edit() ), actionCollection(), "annotate" );
}

DirDetailView::~DirDetailView()
{
//  m_widget->editDone(); BAD IDEA (launching a singleshot timer -> calls slot of dead object ;-)
}

bool DirDetailView::openURL( const KURL &url )
{
  m_metaDataProvider->openDir( url );
  m_currentFileItemSelection.clear();
  m_currentSelection.clear();

  bool res = openURL( url, KonqFileItemList() );
  m_url = url;
  return res;
}

bool DirDetailView::openURL( const KURL &url, KonqFileItemList selection )
{
//  kdDebug() <<  "DirDetailView::openURL( " << url.url() << " ) " << endl;

  m_widget->editDone();

  bool update = false;

  if ( ( selection.count() == 0 && ( m_currentSelection.count() != 0 || m_url != url || forceUpdate ) ) ||
       ( selection.count() > 1 ) )
  {
    QPixmap pix = m_factory->dirMimeType()->pixmap( url, KIcon::Desktop );

    m_widget->setPixmap( pix );

    QDomElement annotation = m_metaDataProvider->metaData( url, "inode/directory", "annotation" );

    if ( !annotation.isNull() && !annotation.text().isEmpty() )
      m_widget->setText( annotation.text() );
    else
      m_widget->setText( url.prettyURL() );

    m_currentSelection.clear();
    update = true;
    m_currentURL = url;
    m_currentServiceType = "inode/directory";
  }
  else if ( selection.count() == 1 && ( m_currentSelection.count() != 1 || selection.first()->url() != m_currentSelection.first() || forceUpdate ) )
  {
    KonqFileItem *item = selection.first();

    m_widget->setPixmap( item->pixmap( 0, true ) );

    QDomElement annotation = m_metaDataProvider->metaData( item->url(), item->mimetype(), "annotation" );
    
    if ( !annotation.isNull() && !annotation.text().isEmpty() )
      m_widget->setText( annotation.text() );
    else
      m_widget->setText( item->url().prettyURL() );

    m_currentSelection.clear();
    m_currentSelection.append( item->url() );
    update = true;
    m_currentURL = item->url();
    m_currentServiceType = item->mimetype();
  }

  if ( update )
    m_widget->update();

  forceUpdate = false;

  return true;
}

bool DirDetailView::eventFilter( QObject *, QEvent *event )
{
  if ( !KonqFileSelectionEvent::test( event ) )
    return false;

  KonqFileSelectionEvent *ev = static_cast<KonqFileSelectionEvent *>( event );

  if ( static_cast<QObject *>( ev->part() ) != parent() )
    return false;

  m_currentFileItemSelection = ev->selection();
  openURL( m_url, ev->selection() );

  return false;
}

void DirDetailView::saveAnnotation( const QString &text )
{
  QDomElement annotation = m_metaDataProvider->metaData( m_currentURL, m_currentServiceType, "annotation" );
  if ( annotation.isNull() )
    return; //ouch
  
  QDomNode n = annotation.firstChild();
  while ( !n.isNull() )
  {
    annotation.removeChild( n );
    n = annotation.firstChild();
  }
  
  // HACK to get stuff like linefeeds correct
  QDomCDATASection textNode = annotation.ownerDocument().createCDATASection( text );
  annotation.appendChild( textNode );
  
  m_metaDataProvider->commit();
  QTimer::singleShot( 0, this, SLOT( slotUpdate() ) );
}

void DirDetailView::slotUpdate()
{
  forceUpdate = true;
  openURL( m_url, m_currentFileItemSelection );
}

#include "konq_dirmetaview.moc"
