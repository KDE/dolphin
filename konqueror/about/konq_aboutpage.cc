#include "konq_aboutpage.h"

#include <qfile.h>
#include <qtextcodec.h>
#include <qtextstream.h>

#include <kaboutdata.h>
#include <kapp.h>
#include <kdebug.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <kinstance.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksavefile.h>
#include <kstddirs.h>
#include <kaction.h>

#include <assert.h>

extern "C"
{
    void *init_libkonqaboutpage()
    {
        return new KonqAboutPageFactory;
    }
}

KInstance *KonqAboutPageFactory::s_instance = 0;
QString *KonqAboutPageFactory::s_intro_html = 0;
QString *KonqAboutPageFactory::s_specs_html = 0;
QString *KonqAboutPageFactory::s_tips_html = 0;

KonqAboutPageFactory::KonqAboutPageFactory( QObject *parent, const char *name )
    : KParts::Factory( parent, name )
{
    s_instance = new KInstance( "konqaboutpage" );
}

KonqAboutPageFactory::~KonqAboutPageFactory()
{
    delete s_instance;
    s_instance = 0;
    delete s_intro_html;
    s_intro_html = 0;
    delete s_specs_html;
    s_specs_html = 0;
    delete s_tips_html;
    s_tips_html = 0;
}

KParts::Part *KonqAboutPageFactory::createPartObject( QWidget *parentWidget, const char *widgetName,
                                                      QObject *parent, const char *name,
                                                      const char *, const QStringList & )
{
    //KonqFrame *frame = dynamic_cast<KonqFrame *>( parentWidget );
    //if ( !frame ) return 0;

    return new KonqAboutPage( //frame->childView()->mainWindow(),
                              parentWidget, widgetName, parent, name );
}

QString KonqAboutPageFactory::loadFile( const QString& file )
{
    QString res;
    if ( file.isEmpty() )
	return res;

    QFile f( file );

    if ( !f.open( IO_ReadOnly ) )
	return res;

    QTextStream t( &f );

    res = t.read();

    // otherwise all embedded objects are referenced as about:/...
    QString basehref = QString::fromLatin1("<BASE HREF=\"file:") +
		       file.left( file.findRev( '/' )) +
		       QString::fromLatin1("/\">\n");
    res.prepend( basehref );
    return res;
}

QString KonqAboutPageFactory::intro()
{
    if ( s_intro_html )
        return *s_intro_html;

    QString res = loadFile( locate( "data", "konqueror/about/intro.html" ));
    if ( res.isEmpty() )
	return res;

    res = res.arg( i18n("Conquer your Desktop!") )
          .arg( i18n("Please enter a term or an address to be searched on the Internet.") )
          .arg( i18n( "Introduction" ) )
          .arg( i18n( "Tips" ) )
          .arg( i18n( "Specifications" ) )
          .arg( i18n( "Introduction" ) )
          .arg( i18n( "Welcome to Konqueror %1." ).arg( KGlobal::instance()->aboutData()->version() ) )
          .arg( i18n( "With Konqueror you have your filesystem at your command, browsing "
		      "local or networked drives with equal ease. Thanks to the component "
		      "technology used throughout KDE 2, Konqueror is also a full featured, "
		      "easy to use and comfortable Web Browser, which you can use to explore "
		      "the Internet." ) )
          .arg( i18n( "Simply enter the Internet address (e.g. <A HREF=\"http://www.kde.org\">http://www.kde.org</A>) "
                      "of the web page you want and press Enter, or choose one of the "
		      "entries in your Bookmarks menu.") )
          .arg( i18n( "If you want to go back to the previous "
		      "web page, press the button <IMG WIDTH=16 HEIGHT=16 SRC=\"%1\"> &nbsp;"
                      "(\"Back\") in the toolbar.").arg("back.png") )
          .arg( i18n( "To go back to the home directory of your local filesystem press "
                      "<IMG WIDTH=16 HEIGHT=16 SRC=\"%1\"> &nbsp; (\"Home\"). " ).arg("gohome.png") )
          .arg( i18n( "For more detailed documentation on Konqueror click <A HREF=\"%1\">here</A>." )
                      .arg("exec:/khelpcenter") )
          .arg( i18n( "<I>Tuning Tip:</I> If you want the Konqueror web browser to start faster,"
			" you can turn off this information screen by clicking <A HREF=\"%1\">here</A>. You can re-enable it"
			" by choosing the Help -> Konqueror Introduction menu option, and then pressing "
			"Window -> Save View Profile \"Web Browsing\".").arg("config:/disable_overview") )
          .arg( i18n( "Continue" ) )
          ;


    s_intro_html = new QString( res );

    return res;
}

QString KonqAboutPageFactory::specs()
{
    if ( s_specs_html )
        return *s_specs_html;

    QString res = loadFile( locate( "data", "konqueror/about/specs.html" ));
    if ( res.isEmpty() )
	return res;

    res = res.arg( i18n("Conquer your Desktop!") )
          .arg( i18n("Please enter a term or an address to be searched on the Internet.") )
          .arg( i18n("Introduction") )
          .arg( i18n("Tips") )
          .arg( i18n("Specifications") )
          .arg( i18n("Specifications") )
          .arg( i18n("Konqueror is designed to embrace and support Internet standards. "
		     "The aim is to fully implement the officially sanctioned standards "
		     "from organizations such as the W3 and OASIS, while also adding "
		     "extra support for other common usability features that arise as "
		     "de facto standards across the Internet. Along with this support, "
		     "for such functions as favicons, Internet Keywords, and <A HREF=\"%1\">XBEL bookmarks</A>, "
                     "Konqueror also implements:").arg("http://pyxml.sourceforge.net/topics/xbel/") )
          .arg( i18n("W E B B R O W S I N G") )
          .arg( i18n("Supported standards") )
          .arg( i18n("Additional requirements*") )
          .arg( i18n("<A HREF=\"%1\">DOM</A> (Level 1, partially Level 2) based "
                     "<A HREF=\"%2\">HTML 4.01</A>").arg("http://www.w3.org/DOM").arg("http://www.w3.org/TR/html4/") )
          .arg( i18n("built-in") )
          .arg( i18n("<A HREF=\"%1\">Cascading Style Sheets</A> (CSS 1, partially CSS 2)").arg("http://www.w3.org/Style/CSS/") )
          .arg( i18n("built-in") )
          .arg( i18n("<A HREF=\"%1\">ECMA-262</A> Edition 3 (roughly equals JavaScript 1.5)").arg("http://www.ecma.ch/ecma1/STAND/ECMA-262.HTM") )
          .arg( i18n("JavaScript disabled (globally). Enable JavaScript <A HREF=\"%1\">here</A>.").arg("exec:/kcmshell konqhtml") )
          .arg( i18n("JavaScript enabled (globally). Configure JavaScript <A HREF=\\\"%1\\\">here</A>.").arg("exec:/kcmshell konqhtml") ) // leave the double backslashes here, they are necessary for javascript !
          .arg( i18n("Secure <A HREF=\"%1\">Java</A><SUP>&reg;</SUP> support").arg("http://java.sun.com") )
          .arg( i18n("JDK 1.2.0 (Java 2) compatible VM (<A HREF=\"%1\">Blackdown</A>, <A HREF=\"%2\">IBM</A> or <A HREF=\"%3\">Sun</A>)")
                      .arg("http://www.blackdown.org").arg("http://www.ibm.com").arg("http://java.sun.com") )
          .arg( i18n("Enable Java (globally) <A HREF=\"%1\">here</A>.").arg("exec:/kcmshell konqhtml") ) // TODO Maybe test if Java is enabled ?
          .arg( i18n("Netscape Communicator<SUP>&reg;</SUP> plugins (for viewing <A HREF=\"%1\">Flash</A>, <A HREF=\"%2\">Real</A>Audio, <A HREF=\"%3\">Real</A>Video, etc.)")
                      .arg("http://www.macromedia.com/shockwave/download/index.cgi?P1_Prod_Version=ShockwaveFlash")
                      .arg("http://www.real.com").arg("http://www.real.com") )
          .arg( i18n("OSF/Motif<SUP>&reg;</SUP>-compatible library (<A HREF=\"%1\">Open Motif</A> or <A HREF=\"%2\">LessTif</A>)")
                      .arg("http://www.openmotif.com").arg("http://www.lesstif.org") )
          .arg( i18n("Secure Sockets Layer") )
          .arg( i18n("(TLS/SSL v2/3) for secure communications up to 168bit") )
          .arg( i18n("OpenSSL") )
          .arg( i18n("Bidirectional 16bit unicode support") )
          .arg( i18n("built-in") )
          .arg( i18n("AutoCompletion for forms") )
          .arg( i18n("built-in") )
          .arg( i18n("G E N E R A L") )
          .arg( i18n("Feature") )
          .arg( i18n("Details") )
          .arg( i18n("Image formats") )
          .arg( i18n("Transfer protocols") )
          .arg( i18n("HTTP 1.1 (including gzip/bzip2 compression)") )
          .arg( i18n("FTP") )
          .arg( i18n("and <A HREF=\"%1\">many more...</A>").arg("exec:/kcmshell ioslaveinfo") )
          .arg( i18n("URL-Completion") )
          .arg( i18n("Manual"))
	  .arg( i18n("Popup"))
	  .arg( i18n("(Short-) Automatic"))
          .arg( i18n("<A HREF=\"%1\">Back</A> to the Introduction").arg("intro.html") )

          ;

    s_specs_html = new QString( res );

    return res;
}

QString KonqAboutPageFactory::tips()
{
    if ( s_tips_html )
        return *s_tips_html;

    QString res = loadFile( locate( "data", "konqueror/about/tips.html" ));
    if ( res.isEmpty() )
	return res;

    res = res.arg( i18n("Conquer your Desktop!") )
          .arg( i18n("Please enter a term or an address to be searched on the Internet.") )
	  .arg( i18n( "Introduction" ) )
	  .arg( i18n( "Tips" ) )
	  .arg( i18n( "Specifications" ) )
	  .arg( i18n( "Tips" ) )
	  .arg( i18n( "Use Internet-Keywords and Web-Shortcuts! By typing \"gg: KDE\" one can search the Internet, "
		      "using Google, for the search phrase \"KDE\". There are alot of "
		      "Web-Shortcuts predefined to make searching for software or looking "
		      "up certain words in an encyclopedia a breeze. You can even "
                      "<A HREF=\"%1\">create your own</A> Web-Shortcuts!" ).arg("exec:/kcmshell ebrowsing") )
	  .arg( i18n( "Use the magnifier button <IMG WIDTH=16 HEIGHT=16 SRC=\"%1\"> &nbsp; in the"
		      " toolbar to increase the font size on your web page.").arg("viewmag.png") )
	  .arg( i18n( "When you want to paste a new address into the Location toolbar you might want to "
		      "clear the current entry by pressing the black arrow with the white cross &nbsp;"
		      "<IMG WIDTH=16 HEIGHT=16 SRC=\"%1\"> &nbsp; in the toolbar.").arg("locationbar_erase.png"))
	  .arg( i18n( "To create a link on your desktop pointing to the current page, "
		      "simply drag the \"Location\" label that is to the left of the Location toolbar, drop it on to "
		      "the desktop, and choose \"Link\"." ) )
	  .arg( i18n( "You can also find <IMG WIDTH=16 HEIGHT=16 SRC=\"%1\"> &nbsp; \"Full-Screen Mode\" "
		      "in the Window menu. This feature is very useful for \"Talk\" "
		      "sessions.").arg("window_fullscreen.png") )
	  .arg( i18n( "Divide et impera (lat. \"Divide and conquer\") - by splitting a window "
		      "into two parts (e.g. Window -> <IMG WIDTH=16 HEIGHT=16 SRC=\"%1\"> &nbsp; Split View "
		      "Left/Right) you can make Konqueror appear the way you like. You"
		      " can even load some example view-profiles (e.g. Midnight Commander)"
		      ", or create your own ones." ).arg("view_left_right.png"))
	  .arg( i18n( "Use the <A HREF=\"%1\">user-agent</A> feature if the website you're visiting "
                      "asks you to use a different browser "
		      "(and don't forget to send a complaint to the webmaster!)" ).arg("exec:/kcmshell useragent") )
	  .arg( i18n( "The <IMG WIDTH=16 HEIGHT=16 SRC=\"%1\"> &nbsp; History in your SideBar ensures "
		      "that you can keep track of the pages you have visited recently.").arg("history.png") )
	  .arg( i18n( "Use a caching <A HREF=\"%1\">proxy</A> to speed up your"
		      " Internet connection.").arg("exec:/kcmshell proxy") )
	  .arg( i18n( "Advanced users will appreciate the Konsole which you can embed into "
		      "Konqueror (Window -> <IMG WIDTH=16 HEIGHT=16 SRC=\"%1\"> &nbsp; Show "
 		      "Terminal Emulator).").arg("openterm.png"))
	  .arg( i18n( "Thanks to <A HREF=\"%1\">DCOP</A> you can have full control over Konqueror using a script."
).arg("exec:/kdcop") )
	  .arg( i18n( "Continue" ) )
          ;


    s_tips_html = new QString( res );

    return res;
}


KonqAboutPage::KonqAboutPage( //KonqMainWindow *
                              QWidget *parentWidget, const char *widgetName,
                              QObject *parent, const char *name )
    : KHTMLPart( parentWidget, widgetName, parent, name, BrowserViewGUI )
{
    //m_mainWindow = mainWindow;
    QTextCodec *codec = QTextCodec::codecForName(KGlobal::locale()->charset().latin1());
    if (codec)
	setCharset(codec->name(), true);
    else
	setCharset(KGlobal::locale()->charset(), true);
    // about:blah isn't a kioslave -> disable View source
    KAction * act = actionCollection()->action("viewDocumentSource");
    if ( act )
      act->setEnabled( false );
}

KonqAboutPage::~KonqAboutPage()
{
}

bool KonqAboutPage::openURL( const KURL & )
{
    serve( KonqAboutPageFactory::intro() );
    return true;
}

bool KonqAboutPage::openFile()
{
    return true;
}

void KonqAboutPage::saveState( QDataStream &stream )
{
    stream << m_htmlDoc;
}

void KonqAboutPage::restoreState( QDataStream &stream )
{
    stream >> m_htmlDoc;
    serve( m_htmlDoc );
}

void KonqAboutPage::serve( const QString& html )
{
    begin( "about:konqueror" );
    write( html );
    end();
    m_htmlDoc = html;
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

    if ( url == QString::fromLatin1("intro.html") )
    {
        emit browserExtension()->openURLNotify();
	serve( KonqAboutPageFactory::intro() );
        return;
    }
    else if ( url == QString::fromLatin1("specs.html") )
    {
        emit browserExtension()->openURLNotify();
	serve( KonqAboutPageFactory::specs() );
        return;
    }
    else if ( url == QString::fromLatin1("tips.html") )
    {
        emit browserExtension()->openURLNotify();
        serve( KonqAboutPageFactory::tips() );
        return;
    }

    else if ( url == QString::fromLatin1("config:/disable_overview") )
    {
	if ( KMessageBox::questionYesNo( widget(), 
					 i18n("Do you want to disable showing\n"
					      "the Introduction in the webbrowsing profile?"),
					 i18n("Faster startup?") )
	     == KMessageBox::Yes ) 
	{
	    QString profile = locateLocal("data", "konqueror/profiles/webbrowsing");
	    KSaveFile file( profile );
	    if ( file.status() == 0 ) {
		QCString content = "[Profile]\n"
			           "Name=Web-Browser";
		fputs( content.data(), file.fstream() );
		file.close();
	    }
	}
	return;
    }
    
    KHTMLPart::urlSelected( url, button, state, target );
}

#include "konq_aboutpage.moc"
