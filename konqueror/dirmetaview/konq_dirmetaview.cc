
#include "konq_dirmetaview.h"
#include "konq_events.h"

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
//  m_bg = KonqFactory::instance()->iconLoader()->loadIcon( "/home/shaus/tt2.png" );
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

  r.moveBy( 5, 5 );
  r.setWidth( r.width() - 5 );
  r.setHeight( r.height() - 5 );
  /*
  int x = r.right() - m_pix.width() - 5;
  int y1 = r.top() + 4;
  int y2 = r.bottom() - 4;

  painter->setPen( colorGroup().shadow() );
  painter->drawLine( x, y1, x, y2 );
  x--;
  painter->setPen( colorGroup().light() );
  painter->drawLine( x, y1, x, y2 );

  painter->drawPixmap( r.right() - m_pix.width(), ( r.bottom() / 2 ) - ( m_pix.height() / 2 ), m_pix );
  */

  int pixWidth = m_pix.width();

  if ( m_secondPix.isNull() )
  {
    painter.drawPixmap( r.left(), ( r.bottom() / 2 ) - 32, m_pix );
  }
  else
  {
    int x = r.left();
    int y = ( r.bottom() / 2 ) - 32;
    painter.drawPixmap( x, y, m_pix );

    x += 32;
    y += 16;
    painter.drawPixmap( x, y, m_secondPix );
    if ( !m_thirdPix.isNull() )
    {
      x -= 16;
      y += 16;
      painter.drawPixmap( x, y, m_thirdPix );
    }
    pixWidth = 64;
  }


  r.moveBy( pixWidth + 5, 0 );

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

  if ( m_stext.isEmpty() )
  {
    painter.setPen( colorGroup().text() );
    QFont f = font();
    f.setBold( true );
    f.setPointSize( f.pointSize() + 6 );
    painter.setFont( f );
    //    painter->setPen( colorGroup().text() );
    //    painter->drawText( r, AlignLeft | AlignVCenter, m_text );
  }

  QRect firstColRect;
  painter.drawText( r, AlignLeft | AlignVCenter, m_text, -1, &firstColRect );

  QRect secondColRect( r );
  secondColRect.moveBy( firstColRect.width() + 5, 0 );
  if ( !m_stext.isEmpty() )
  {
    painter.setFont( font() );
    painter.drawText( secondColRect, AlignLeft | AlignVCenter, m_stext );
  }

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
}

DirDetailView::~DirDetailView()
{
}

bool DirDetailView::openURL( const KURL &url )
{
/*
  m_metaDataURL = url;
  m_metaDataURL.addPath( ".kdemetadata" );

  // ### HACK :-( I wish there'd be KIO::accessJob ;-)
  m_bEditableMetaData = checkAccess( m_metaDataURL.path(), W_OK );

  m_metaData = QDomDocument();
  if ( QFile::exists( m_metaDataURL.path() ) )
  {
    QFile metaDataFile( m_metaDataURL.path() );
    if ( metaDataFile.open( IO_ReadOnly ) )
    {
      m_metaData = QDomDocument( &metaDataFile );
      metaDataFile.close();
    }
  }
*/
  bool res = openURL( url, KonqFileItemList() );
  m_url = url;
  return res;
}

bool DirDetailView::openURL( const KURL &url, KonqFileItemList selection )
{
//  kdDebug() <<  "DirDetailView::openURL( " << url.url() << " ) " << endl;

  bool update = false;

  //  kdDebug() << "selection: " << selection.count() << " -- previous: " << m_currentSelection.count() << endl;

  if ( selection.count() == 0 && ( m_currentSelection.count() != 0 || m_url != url ) )
  {
    QPixmap pix = m_factory->dirMimeType()->pixmap( url, KIconLoader::Large );

    //    QString annotation = annotationMetaData( m_url );

    m_widget->setPixmap( pix );
    //    if ( annotation.isEmpty() )
      m_widget->setText( url.decodedURL() );
      //    else
      //      m_widget->setText( annotation );
    m_widget->setSecondColumnText( QString::null );

    m_currentSelection.clear();
    update = true;
  }
  else if ( selection.count() == 1 && ( m_currentSelection.count() != 1 || selection.first()->url() != m_currentSelection.first() ) )
  {
    KonqFileItem *item = selection.first();

    m_widget->setPixmap( item->pixmap( KIconLoader::Large, true ) );

    QString txt;

    QString txt2 = item->text() + '\n';

    if ( S_ISDIR( item->mode() ) )
    {
      txt = i18n( "directory\n\ndate" );
      txt2 += '\n';
    }
    else
    {
      txt = i18n( "filename\nsize\ndate" );
      txt2 += KIO::convertSize( item->size() ) + '\n';
    }

    txt2 += item->time( KIO::UDS_MODIFICATION_TIME );

    m_widget->setText( txt );
    m_widget->setSecondColumnText( txt2 );

    m_currentSelection.clear();
    m_currentSelection.append( item->url() );
    update = true;
  }
  else
  {
    bool changed = false;

    if ( m_currentSelection.count() == selection.count() )
    {
      KonqFileItemListIterator it( selection );
      KURL::List::ConstIterator selIt = m_currentSelection.begin();
      for (; it.current(); ++it, ++selIt )
        if ( *selIt != it.current()->url() )
	{
	  changed = true;
	  break;
	}
    }
    else
      changed = true;

    if ( changed )
    {
      QPixmap pixmaps[3];
      QStringList mTypes;

      KonqFileItemListIterator it( selection );
      int i = 0;
      while ( i < 3 && it.current() )
      {
        QString type = it.current()->mimetype();
	
        if ( !mTypes.contains( type ) )
	{
          pixmaps[i] = it.current()->pixmap( KIconLoader::Medium, true );
	
  	  mTypes.append( type );
	  ++i;
	}
	
        ++it;
      }

      m_widget->setPixmap( pixmaps[0], pixmaps[1], pixmaps[2] );

      uint dirCount = 0;
      uint fileCount = 0;
      uint totalSize = 0;

      m_currentSelection.clear();

      it.toFirst();
      for (; it.current(); ++it )
      {
        if ( S_ISDIR( it.current()->mode() ) )
	  dirCount++;
        else
	{
	  fileCount++;
	  totalSize += it.current()->size();
	}
	m_currentSelection.append( it.current()->url() );
      }

      QString totalSizeStr = KIO::convertSize( totalSize );

      QStringList totalSizeStrLst = QStringList::split( ' ', totalSizeStr );

      assert( totalSizeStrLst.count() == 2 );

      QString txt = QString::number( selection.count() ).append( '\n' );
      txt += totalSizeStrLst[ 0 ];

      m_widget->setText( txt );

      txt = i18n( "items selected" ).append( "( " );

      if ( fileCount == 1 )
        txt.append( i18n( "one file" ) );
      else
        txt.append( i18n( "%1 files" ).arg( fileCount ) );

      txt.append( " / " );

      if ( dirCount == 1 )
        txt.append( i18n( "one directory" ) );
      else
        txt.append( i18n( "%1 directories" ).arg( dirCount ) );

      txt += " )\n";
      txt += totalSizeStrLst[ 1 ];

      m_widget->setSecondColumnText( txt );

      update = true;
    }
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

void DirDetailView::setAnnotationMetaData( const KURL &url, const QString &text )
{
}

QString DirDetailView::annotationMetaData( const KURL &url )
{
  QString empty;

  QDomElement docElement = m_metaData.documentElement();

  if ( docElement.isNull() )
    return empty;

  if ( m_url == url )
  {
    QDomElement annotationElement = docElement.namedItem( "annotation" ).toElement();
    if ( annotationElement.isNull() )
      return empty;

    return annotationElement.text();
  }

  QDomElement it = docElement.firstChild().toElement();
  for (; !it.isNull(); it = it.nextSibling().toElement() )
  {
    if ( it.tagName() != "item" )
      continue;

    QDomElement urlElement = it.namedItem( "url" ).toElement();
    if ( urlElement.isNull() )
      continue;

    KURL itemURL( urlElement.text() );
    if ( itemURL == url )
    {
      QDomElement annotationElement = it.namedItem( "annotation" ).toElement();
      if ( annotationElement.isNull() )
        return empty;

      return annotationElement.text();
    }
  }

  return empty;
}

#include "konq_dirmetaview.moc"
