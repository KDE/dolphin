
#include "konq_aboutpage.h"
#include "konq_mainwindow.h"
#include "konq_frame.h"
#include "konq_view.h"

#include <qvbox.h>
#include <qtimer.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kinstance.h>
#include <khtml_part.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kdebug.h>

#include <assert.h>

extern "C"
{
    void *init_libkonqaboutpage()
    {
        return new KonqAboutPageFactory;
    }
}

KInstance *KonqAboutPageFactory::s_instance = 0;
QString *KonqAboutPageFactory::s_page = 0;

KonqAboutPageFactory::KonqAboutPageFactory( QObject *parent, const char *name )
    : KParts::Factory( parent, name )
{
    s_instance = new KInstance( "konqaboutpage" );
    s_page = 0;
}

KonqAboutPageFactory::~KonqAboutPageFactory()
{
    delete s_instance;
    s_instance = 0;
    delete s_page;
    s_page = 0;
}

KParts::Part *KonqAboutPageFactory::createPartObject( QWidget *parentWidget, const char *widgetName,
                                                      QObject *parent, const char *name,
                                                      const char *, const QStringList & )
{
    KonqFrame *frame = dynamic_cast<KonqFrame *>( parentWidget );

    if ( !frame )
        return 0;

    return new KonqAboutPage( frame->childView()->mainWindow(),
                              parentWidget, widgetName, parent, name );
}

QString KonqAboutPageFactory::aboutPage()
{
    if ( s_page )
        return *s_page;

    QString res;

    QString path = locate( "data", "konqueror/konq_about.html.template" );

    if ( path.isEmpty() )
        return res; // ugh

    QFile f( path );

    if ( !f.open( IO_ReadOnly ) )
        return res;

    QByteArray data = f.readAll();

    f.close();

    data.resize( data.size() + 1 );
    data[ data.size() - 1 ] = 0;

    res = QString::fromLatin1( data.data() );

    res = res.arg( i18n( "Insert the URL you want to browse in the above edit-field." ) )
          .arg( i18n( "" ) )
          .arg( i18n( "In addition to <a href=\"http://grail.sourceforge.net/info/xbel.html\">XBEL-bookmarks</a>,"
                      "favourite-icon support and Internet Keywords Konqueror supports:" ) )
          .arg( i18n( "Specifications" ) )
          .arg( i18n( "Supported standards" ) )
          .arg( i18n( "Additional requirements" ) )
          .arg( i18n( "<a href=\"http://www.w3.org/DOM/\">DOM</a> (Level 1, partially Level 2) based "
                      "<a href=\"http://www.w3.org/TR/html4/\">HTML 4.01</a>" ) )
          .arg( i18n( "built-in" ) )
          .arg( i18n( "<a href=\"http://www.w3.org/Style/CSS/\">Cascading Style Sheets</a> (CSS 1, partially CSS2)" ) )
          .arg( i18n( "built-in" ) )
          .arg( i18n( "<a href=\"http://www.ecma.ch/ecma1/STAND/ECMA-262.HTM\">ECMA-262</a>"
                      "Edition 3 (equals roughly Javascript<sup>TM</sup> 1.5" ) )
          .arg( i18n( "Enable Javascript (globally) <a href=\"\">here</a>" ) )
          .arg( i18n( "Secure <a href=\"http://java.sun.com\">Java</a><sup>&reg;</sup> support" ) )
	  .arg( i18n( "JDK 1.2.0 (Java 2) compatible VM (<A HREF=\"http://www.blackdown.org\">Blackdown</A>, <A HREF=\"\">IBM</A>, <A HREF=\"http://www.kaffe.org\">Kaffe</A> or <A HREF=\"http://java.sun.com\">Sun</A>)" ) )
	  .arg( i18n( "Enable Java (globally) <A HREF=\"\">here</A>" ) )
	  .arg( i18n( "Netscape Communicator<SUP>&reg;</SUP> plugins (for viewing <A HREF=\"\">Flash</A><SUP>TM</SUP>, <A HREF=\"http://www.real.com\">Real</A>Audio<SUP>TM</SUP>, RealVideo<SUP>TM</SUP> etc.)" ) )
	  .arg( i18n( "OSF/Motif<SUP>&reg;</SUP>-compatible library (<A HREF=\"http://www.openmotif.com\">Open Motif</A> or <A HREF=\"http://www.lesstif.org\">LessTif</A>)" ) )
	  .arg( i18n( "<A HREF=\"http://www.netscape.com/eng/ssl3/\">Secure Sockets Layer</A> (TLS/SSL v2/3) for secure communications up to 168bit" ) )
	  .arg( i18n( "<A HREF=\"http://www.openssl.org\">OpenSSL</A>" ) )
	  .arg( i18n( "Bidirectional 16bit unicode support" ) )
	  .arg( i18n( "built-in" ) );
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) )
// 	  .arg( i18n( "" ) );


    s_page = new QString( res );

    return res;
}

KonqAboutPage::KonqAboutPage( KonqMainWindow *mainWindow,
                              QWidget *parentWidget, const char *widgetName,
                              QObject *parent, const char *name )
    : KHTMLPart( parentWidget, widgetName, parent, name )
{
    m_mainWindow = mainWindow;

//    setInstance( KonqAboutPageFactory::instance(), false );

//    QVBox *box = new QVBox( parentWidget, widgetName );

//    m_doc = new KHTMLPart( box, "docview", this, "doc" );

//    setWidget( box );
}

KonqAboutPage::~KonqAboutPage()
{
//    delete m_doc;
}

bool KonqAboutPage::openURL( const KURL &url )
{
//    m_doc->begin( url );
//    m_doc->write( "<html><body><h1>*test*</h1></body></html>" );
//    m_doc->end();
//    m_doc->openURL( "file:/home/simon/about.html" );
//    KHTMLPart::openURL( "file:/home/simon/about.html" );
    begin( url );
    write( KonqAboutPageFactory::aboutPage() );
    end();

    QTimer::singleShot( 0, m_mainWindow->action( "clear_location" ), SLOT( activate() ) );

    return true;
}

bool KonqAboutPage::openFile()
{
    return true;
}

void KonqAboutPage::saveState( QDataStream &stream )
{
    browserExtension()->KParts::BrowserExtension::saveState( stream );
}

void KonqAboutPage::restoreState( QDataStream &stream )
{
    browserExtension()->KParts::BrowserExtension::restoreState( stream );
}

#include "konq_aboutpage.moc"
