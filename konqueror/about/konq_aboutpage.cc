
#include "konq_aboutpage.h"
#include "konq_mainwindow.h"
#include "konq_frame.h"
#include "konq_view.h"

#include <qvbox.h>
#include <qtimer.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kapp.h>
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

    QString path = locate( "data", "konqueror/about/intro.html" );

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
    
    // otherwise all relative URLs are referenced as about:/...
    QString basehref = QString::fromLatin1("<BASE HREF=\"file:") + 
		       path.left( path.findRev( '/' )) + 
		       QString::fromLatin1("/\">\n");
    res.prepend( basehref );

    
    QString kcmshell_konqhtml = QString::fromLatin1("exec:/kcmshell konqhtml");
    QString kcmshell_ioslaveinfo = QString::fromLatin1("exec:/kcmshell ioslaveinfo");

    return res; // testing
    
    res = res.arg( i18n( "Insert the URL you want to browse in the above edit-field." ) )
          .arg( "" ) // TODO Konqueror-Logo
          .arg( i18n( "Konqueror supports:" ) )
          .arg( i18n( "Specifications" ) )
          .arg( i18n( "Supported standards" ) )
          .arg( i18n( "Additional requirements" ) )
          .arg( i18n( "<a href=\"%1\">DOM</a> (Level 1, partially Level 2) based "
                      "<a href=\"%2\">HTML 4.01</a>" ).arg("http://www.w3.org/DOM/").arg("http://www.w3.org/TR/html4/") )
          .arg( i18n( "built-in" ) )
          .arg( i18n( "<a href=\"%1\">Cascading Style Sheets</a> (CSS 1, partially CSS2)" ).arg("http://www.w3.org/Style/CSS/") )
          .arg( i18n( "built-in" ) )
          .arg( i18n( "<a href=\"%1\">ECMA-262</a> "
                      "Edition 3 (equals roughly Javascript<sup>TM</sup> 1.5)" ).arg("http://www.ecma.ch/ecma1/STAND/ECMA-262.HTM") )
          .arg( i18n( "Javascript disabled (globally). Enable Javascript <a href=\"%1\">here</a>" ).arg(kcmshell_konqhtml) )
          .arg( i18n( "Javascript enabled (globally). Javascript configuration <a href=\\\"%1\\\">here</a>" ).arg(kcmshell_konqhtml) )
          .arg( i18n( "Secure <a href=\"%1\">Java</a><sup>&reg;</sup> support" ).arg("http://java.sun.com") )
	  .arg( i18n( "JDK 1.2.0 (Java 2) compatible VM (<A HREF=\"%1\">Blackdown</A>, <A HREF=\"%2\">IBM</A>, <A HREF=\"%3\">Kaffe</A> or <A HREF=\"%4\">Sun</A>)" ).arg("http://www.blackdown.org").arg("http://www.ibm.com").arg("http://www.kaffe.org").arg("http://java.sun.com") )
	  .arg( i18n( "Enable Java (globally) <A HREF=\"%1\">here</A>" ).arg(kcmshell_konqhtml) )
	  .arg( i18n( "Netscape Communicator<SUP>&reg;</SUP> plugins (for viewing <A HREF=\"%1\">Flash</A><SUP>TM</SUP>, <A HREF=\"%2\">Real</A>Audio<SUP>TM</SUP>, <A HREF=\"%2\">Real</A>Video<SUP>TM</SUP> etc.)" ).arg("TODO").arg("http://www.real.com").arg("http://www.real.com") )
	  .arg( i18n( "OSF/Motif<SUP>&reg;</SUP>-compatible library (<A HREF=\"%1\">Open Motif</A> or <A HREF=\"%2\">LessTif</A>)" ).arg("http://www.openmotif.com").arg("http://www.lesstif.org") )
	  .arg( i18n( "<A HREF=\"%1\">Secure Sockets Layer</A> (TLS/SSL v2/3) for secure communications up to 168bit" ).arg("http://www.netscape.com/eng/ssl3/") )
	  .arg( i18n( "<A HREF=\"%1\">OpenSSL</A>" ).arg("http://www.openssl.org") )
	  .arg( i18n( "Bidirectional 16bit unicode support" ) )
	  .arg( i18n( "built-in" ) )
 	  .arg( i18n( "Image formats:" ) )
 	  .arg( "<LI>PNG</LI><LI>MNG</LI><LI>JPG</LI><LI>GIF</LI>" ) // TODO better than that
	  .arg( i18n( "built-in" ) )
	  .arg( i18n( "Transfer protocols:") )
	  .arg( i18n( "HTTP 1.1 (including gzip/bzip2 compression)" ) )
	  .arg( i18n( "FTP" ) )
          .arg( i18n( "<a href=\"%1\">and many more</a>" ).arg(kcmshell_ioslaveinfo) )
	  .arg( i18n( "built-in" ) )
 	  .arg( i18n( "<a href=\"%1\">XBEL bookmarks</a>" ).arg("http://pyxml.sourceforge.net/topics/xbel/") )
	  .arg( i18n( "built-in" ) )
 	  .arg( i18n( "Favourite icon support" ) )
	  .arg( i18n( "built-in" ) )
 	  .arg( i18n( "Internet Keywords" ) )
	  .arg( i18n( "built-in" ) )
          ;


    s_page = new QString( res );

    return res;
}

KonqAboutPage::KonqAboutPage( KonqMainWindow *, // TODO get rid of this
                              QWidget *parentWidget, const char *widgetName,
                              QObject *parent, const char *name )
    : KHTMLPart( parentWidget, widgetName, parent, name )
{
    //m_mainWindow = mainWindow;
}

KonqAboutPage::~KonqAboutPage()
{
}

bool KonqAboutPage::openURL( const KURL & )
{
    begin( "about:konqueror" );
    write( KonqAboutPageFactory::aboutPage() );
    end();
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

void KonqAboutPage::urlSelected( const QString &url, int button, int state, const QString &target )
{
    KURL u( url );
    if ( u.protocol() == "exec" )
    {
        QStringList args = QStringList::split( QChar( ' ' ), url.mid( 6 ) );
        QString executable = args[ 0 ];
        args.remove( args.begin() );
        KApplication::kdeinitExec( executable, args );
        return;
    }

    KHTMLPart::urlSelected( url, button, state, target );
}

#include "konq_aboutpage.moc"
