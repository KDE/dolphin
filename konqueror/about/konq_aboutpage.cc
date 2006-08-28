#include "konq_aboutpage.h"

#include <QTextCodec>
//Added by qt3to4:
#include <QTextStream>

#include <kaboutdata.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <ktoolinvocation.h>

#include <assert.h>
#include <QFile>
#include <QDir>

K_EXPORT_COMPONENT_FACTORY( konq_aboutpage, KonqAboutPageFactory )

KInstance *KonqAboutPageFactory::s_instance = 0;
QString *KonqAboutPageFactory::s_launch_html = 0;
QString *KonqAboutPageFactory::s_intro_html = 0;
QString *KonqAboutPageFactory::s_specs_html = 0;
QString *KonqAboutPageFactory::s_tips_html = 0;
QString *KonqAboutPageFactory::s_plugins_html = 0;

KonqAboutPageFactory::KonqAboutPageFactory( QObject *parent )
    : KParts::Factory( parent )
{
    s_instance = new KInstance( "konqaboutpage" );
}

KonqAboutPageFactory::~KonqAboutPageFactory()
{
    delete s_instance;
    s_instance = 0;
    delete s_launch_html;
    s_launch_html = 0;
    delete s_intro_html;
    s_intro_html = 0;
    delete s_specs_html;
    s_specs_html = 0;
    delete s_tips_html;
    s_tips_html = 0;
    delete s_plugins_html;
    s_plugins_html = 0;
}

KParts::Part *KonqAboutPageFactory::createPartObject( QWidget *parentWidget, QObject *parent,
                                                      const char *, const QStringList & )
{
    //KonqFrame *frame = dynamic_cast<KonqFrame *>( parentWidget );
    //if ( !frame ) return 0;

    return new KonqAboutPage( parentWidget, parent );
}

QString KonqAboutPageFactory::loadFile( const QString& file )
{
    QString res;
    if ( file.isEmpty() )
	return res;

    QFile f( file );

    if ( !f.open( QIODevice::ReadOnly ) )
	return res;

    QTextStream t( &f );

    res = t.readAll();

    // otherwise all embedded objects are referenced as about:/...
    QString basehref = QLatin1String("<BASE HREF=\"file:") +
		       file.left( file.lastIndexOf( '/' )) +
		       QLatin1String("/\">\n");
    QRegExp reg("<head>");
    reg.setCaseSensitivity(Qt::CaseInsensitive);
    res.replace(reg, "<head>\n\t" + basehref);
    return res;
}

QString KonqAboutPageFactory::launch()
{
  if ( s_launch_html )
    return *s_launch_html;

  QString res = loadFile( KStandardDirs::locate( "data", "konqueror/about/launch.html" ));
  if ( res.isEmpty() )
    return res;

  KIconLoader *iconloader = KGlobal::iconLoader();
  int iconSize = iconloader->currentSize(K3Icon::Desktop);
  QString home_icon_path = iconloader->iconPath("folder_home", K3Icon::Desktop );
  QString storage_icon_path = iconloader->iconPath("system", K3Icon::Desktop );
  QString remote_icon_path = iconloader->iconPath("network", K3Icon::Desktop );
  QString wastebin_icon_path = iconloader->iconPath("trashcan_full", K3Icon::Desktop );
  QString applications_icon_path = iconloader->iconPath("kmenu", K3Icon::Desktop );
  QString settings_icon_path = iconloader->iconPath("kcontrol", K3Icon::Desktop );
  QString home_folder = QDir::homePath();
  QString continue_icon_path = QApplication::isRightToLeft()?iconloader->iconPath("1leftarrow", K3Icon::Small ):iconloader->iconPath("1rightarrow", K3Icon::Small );

  res = res.arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage.css" ) );
  if ( kapp->layoutDirection() == Qt::RightToLeft )
    res = res.arg( "@import \"%1\";" ).arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage_rtl.css" ) );
  else
    res = res.arg( "" );

  res = res.arg( i18n("Conquer your Desktop!") )
      .arg( i18n( "Konqueror" ) )
      .arg( i18n("Conquer your Desktop!") )
      .arg( i18n("Konqueror is your file manager, web browser and universal document viewer.") )
      .arg( i18n( "Starting Points" ) )
      .arg( i18n( "Introduction" ) )
      .arg( i18n( "Tips" ) )
      .arg( i18n( "Specifications" ) )
      .arg( home_folder )
      .arg( home_icon_path )
      .arg(iconSize).arg(iconSize)
      .arg( home_folder )
      .arg( i18n( "Home Folder" ) )
      .arg( i18n( "Your personal files" ) )
      .arg( storage_icon_path )
      .arg(iconSize).arg(iconSize)
      .arg( i18n( "Storage Media" ) )
      .arg( i18n( "Disks and removable media" ) )
      .arg( remote_icon_path )
      .arg(iconSize).arg(iconSize)
      .arg( i18n( "Network Folders" ) )
      .arg( i18n( "Shared files and folders" ) )
      .arg( wastebin_icon_path )
      .arg(iconSize).arg(iconSize)
      .arg( i18n( "Trash" ) )
      .arg( i18n( "Browse and restore the trash" ) )
      .arg( applications_icon_path )
      .arg(iconSize).arg(iconSize)
      .arg( i18n( "Applications" ) )
      .arg( i18n( "Installed programs" ) )
      .arg( settings_icon_path )
      .arg(iconSize).arg(iconSize)
      .arg( i18n( "Settings" ) )
      .arg( i18n( "Desktop configuration" ) )
      .arg( continue_icon_path )
      .arg( K3Icon::SizeSmall ).arg( K3Icon::SizeSmall )
      .arg( i18n( "Next: An Introduction to Konqueror" ) )
      ;
  i18n("Search the Web");//i18n for possible future use

  s_launch_html = new QString( res );

  return res;
}

QString KonqAboutPageFactory::intro()
{
    if ( s_intro_html )
        return *s_intro_html;

    QString res = loadFile( KStandardDirs::locate( "data", "konqueror/about/intro.html" ));
    if ( res.isEmpty() )
	return res;

    KIconLoader *iconloader = KGlobal::iconLoader();
    QString back_icon_path = QApplication::isRightToLeft()?iconloader->iconPath("forward", K3Icon::Small ):iconloader->iconPath("back", K3Icon::Small );
    QString gohome_icon_path = iconloader->iconPath("gohome", K3Icon::Small );
    QString continue_icon_path = QApplication::isRightToLeft()?iconloader->iconPath("1leftarrow", K3Icon::Small ):iconloader->iconPath("1rightarrow", K3Icon::Small );

    res = res.arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage.css" ) );
    if ( kapp->layoutDirection() == Qt::RightToLeft )
	res = res.arg( "@import \"%1\";" ).arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage_rtl.css" ) );
    else
	res = res.arg( "" );

    res = res.arg( i18n("Conquer your Desktop!") )
	.arg( i18n( "Konqueror" ) )
	.arg( i18n( "Conquer your Desktop!") )
	.arg( i18n( "Konqueror is your file manager, web browser and universal document viewer.") )
	.arg( i18n( "Starting Points" ) )
	.arg( i18n( "Introduction" ) )
          .arg( i18n( "Tips" ) )
          .arg( i18n( "Specifications" ) )
          .arg( i18n( "Konqueror makes working with and managing your files easy. You can browse "
                      "both local and networked folders while enjoying advanced features "
                      "such as the powerful sidebar and file previews."
		      ) )
          .arg( i18n( "Konqueror is also a full featured and easy to use web browser which you "
                      "can  use to explore the Internet. "
                      "Enter the address (e.g. <a href=\"http://www.kde.org\">http://www.kde.org</A>) "
                      "of a web page you would like to visit in the location bar and press Enter, "
                      "or choose an entry from the Bookmarks menu.") )
          .arg( i18n( "To return to the previous "
		      "location, press the back button  <img width='16' height='16' src=\"%1\"> "
                      "in the toolbar. ",  back_icon_path ) )
          .arg( i18n( "To quickly go to your Home folder press the "
                      " home button <img width='16' height='16' src=\"%1\">." , gohome_icon_path) )
          .arg( i18n( "For more detailed documentation on Konqueror click <a href=\"%1\">here</a>." ,
                      QString("exec:/khelpcenter")) )
          .arg( i18n( "<em>Tuning Tip:</em> If you want the Konqueror web browser to start faster,"
			" you can turn off this information screen by clicking <a href=\"%1\">here</a>. You can re-enable it"
			" by choosing the Help -> Konqueror Introduction menu option, and then pressing "
			"Settings -> Save View Profile \"Web Browsing\".", QString("config:/disable_overview")) )
	  .arg( "<img width='16' height='16' src=\"%1\">" ).arg( continue_icon_path )
	  .arg( i18n( "Next: Tips &amp; Tricks" ) )
	;


    s_intro_html = new QString( res );

    return res;
}

QString KonqAboutPageFactory::specs()
{
    if ( s_specs_html )
        return *s_specs_html;

    KIconLoader *iconloader = KGlobal::iconLoader();
    QString res = loadFile( KStandardDirs::locate( "data", "konqueror/about/specs.html" ));
    QString continue_icon_path = QApplication::isRightToLeft()?iconloader->iconPath("1leftarrow", K3Icon::Small ):iconloader->iconPath("1rightarrow", K3Icon::Small );
    if ( res.isEmpty() )
	return res;

    res = res.arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage.css" ) );
    if ( kapp->layoutDirection() == Qt::RightToLeft )
	res = res.arg( "@import \"%1\";" ).arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage_rtl.css" ) );
    else
	res = res.arg( "" );

    res = res.arg( i18n("Conquer your Desktop!") )
	.arg( i18n( "Konqueror" ) )
	.arg( i18n("Conquer your Desktop!") )
	.arg( i18n("Konqueror is your file manager, web browser and universal document viewer.") )
	.arg( i18n( "Starting Points" ) )
	.arg( i18n( "Introduction" ) )
	.arg( i18n( "Tips" ) )
	.arg( i18n( "Specifications" ) )
          .arg( i18n("Specifications") )
          .arg( i18n("Konqueror is designed to embrace and support Internet standards. "
		     "The aim is to fully implement the officially sanctioned standards "
		     "from organizations such as the W3 and OASIS, while also adding "
		     "extra support for other common usability features that arise as "
		     "de facto standards across the Internet. Along with this support, "
		     "for such functions as favicons, Internet Keywords, and <A HREF=\"%1\">XBEL bookmarks</A>, "
                     "Konqueror also implements:", QString("http://pyxml.sourceforge.net/topics/xbel/")) )
          .arg( i18n("Web Browsing") )
          .arg( i18n("Supported standards") )
          .arg( i18n("Additional requirements*") )
          .arg( i18n("<A HREF=\"%1\">DOM</A> (Level 1, partially Level 2) based "
                     "<A HREF=\"%2\">HTML 4.01</A>", QString("http://www.w3.org/DOM"), QString("http://www.w3.org/TR/html4/")) )
          .arg( i18n("built-in") )
          .arg( i18n("<A HREF=\"%1\">Cascading Style Sheets</A> (CSS 1, partially CSS 2)", QString("http://www.w3.org/Style/CSS/")) )
          .arg( i18n("built-in") )
          .arg( i18n("<A HREF=\"%1\">ECMA-262</A> Edition 3 (roughly equals JavaScript 1.5)", QString("http://www.ecma.ch/ecma1/STAND/ECMA-262.HTM")) )
          .arg( i18n("JavaScript disabled (globally). Enable JavaScript <A HREF=\"%1\">here</A>.", QString("exec:/kcmshell khtml_java_js")) )
          .arg( i18n("JavaScript enabled (globally). Configure JavaScript <A HREF=\\\"%1\\\">here</A>.", QString("exec:/kcmshell khtml_java_js")) ) // leave the double backslashes here, they are necessary for javascript !
          .arg( i18n("Secure <A HREF=\"%1\">Java</A><SUP>&reg;</SUP> support", QString("http://java.sun.com")) )
          .arg( i18n("JDK 1.2.0 (Java 2) compatible VM (<A HREF=\"%1\">Blackdown</A>, <A HREF=\"%2\">IBM</A> or <A HREF=\"%3\">Sun</A>)",
                       QString("http://www.blackdown.org"), QString("http://www.ibm.com"), QString("http://java.sun.com")) )
          .arg( i18n("Enable Java (globally) <A HREF=\"%1\">here</A>.", QString("exec:/kcmshell khtml_java_js")) ) // TODO Maybe test if Java is enabled ?
          .arg( i18n("Netscape Communicator<SUP>&reg;</SUP> <A HREF=\"%4\">plugins</A> (for viewing <A HREF=\"%1\">Flash<SUP>&reg;</SUP></A>, <A HREF=\"%2\">Real<SUP>&reg;</SUP></A>Audio, <A HREF=\"%3\">Real<SUP>&reg;</SUP></A>Video, etc.)",
                       QString("http://www.macromedia.com/shockwave/download/index.cgi?P1_Prod_Version=ShockwaveFlash"),
                       QString("http://www.real.com"), QString("http://www.real.com"),
                       QString("about:plugins")) )
          .arg( i18n("built-in") )
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
          .arg( i18n("and <A HREF=\"%1\">many more...</A>", QString("exec:/kcmshell ioslaveinfo")) )
          .arg( i18n("URL-Completion") )
          .arg( i18n("Manual"))
	  .arg( i18n("Popup"))
	  .arg( i18n("(Short-) Automatic"))
	  .arg( "<img width='16' height='16' src=\"%1\">" ).arg( continue_icon_path )
	  .arg( i18n("<a href=\"%1\">Return to Starting Points</a>", QString("launch.html")) )

          ;

    s_specs_html = new QString( res );

    return res;
}

QString KonqAboutPageFactory::tips()
{
    if ( s_tips_html )
        return *s_tips_html;

    QString res = loadFile( KStandardDirs::locate( "data", "konqueror/about/tips.html" ));
    if ( res.isEmpty() )
	return res;

    KIconLoader *iconloader = KGlobal::iconLoader();
    QString viewmag_icon_path =
	    iconloader->iconPath("viewmag", K3Icon::Small );
    QString history_icon_path =
	    iconloader->iconPath("history", K3Icon::Small );
    QString openterm_icon_path =
	    iconloader->iconPath("openterm", K3Icon::Small );
    QString locationbar_erase_rtl_icon_path =
	    iconloader->iconPath("clear_left", K3Icon::Small );
    QString locationbar_erase_icon_path =
	    iconloader->iconPath("locationbar_erase", K3Icon::Small );
    QString window_fullscreen_icon_path =
	    iconloader->iconPath("window_fullscreen", K3Icon::Small );
    QString view_left_right_icon_path =
	    iconloader->iconPath("view_left_right", K3Icon::Small );
    QString continue_icon_path = QApplication::isRightToLeft()?iconloader->iconPath("1leftarrow", K3Icon::Small ):iconloader->iconPath("1rightarrow", K3Icon::Small );

    res = res.arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage.css" ) );
    if ( kapp->layoutDirection() == Qt::RightToLeft )
	res = res.arg( "@import \"%1\";" ).arg( KStandardDirs::locate( "data", "kdeui/about/kde_infopage_rtl.css" ) );
    else
	res = res.arg( "" );

    res = res.arg( i18n("Conquer your Desktop!") )
	.arg( i18n( "Konqueror" ) )
	.arg( i18n("Conquer your Desktop!") )
	.arg( i18n("Konqueror is your file manager, web browser and universal document viewer.") )
	.arg( i18n( "Starting Points" ) )
	.arg( i18n( "Introduction" ) )
	.arg( i18n( "Tips" ) )
	.arg( i18n( "Specifications" ) )
	.arg( i18n( "Tips &amp; Tricks" ) )
	  .arg( i18n( "Use Internet-Keywords and Web-Shortcuts: by typing \"gg: KDE\" one can search the Internet, "
		      "using Google, for the search phrase \"KDE\". There are a lot of "
		      "Web-Shortcuts predefined to make searching for software or looking "
		      "up certain words in an encyclopedia a breeze. You can even "
                      "<a href=\"%1\">create your own</a> Web-Shortcuts." , QString("exec:/kcmshell ebrowsing")) )
	  .arg( i18n( "Use the magnifier button <img width='16' height='16' src=\"%1\"> in the"
		      " toolbar to increase the font size on your web page.", viewmag_icon_path) )
	  .arg( i18n( "When you want to paste a new address into the Location toolbar you might want to "
		      "clear the current entry by pressing the black arrow with the white cross "
		      "<img width='16' height='16' src=\"%1\"> in the toolbar.",
               QApplication::isRightToLeft() ? locationbar_erase_rtl_icon_path : locationbar_erase_icon_path))
	  .arg( i18n( "To create a link on your desktop pointing to the current page, "
		      "simply drag the \"Location\" label that is to the left of the Location toolbar, drop it on to "
		      "the desktop, and choose \"Link\"." ) )
	  .arg( i18n( "You can also find <img width='16' height='16' src=\"%1\"> \"Full-Screen Mode\" "
		      "in the Settings menu. This feature is very useful for \"Talk\" "
		      "sessions.", window_fullscreen_icon_path) )
	  .arg( i18n( "Divide et impera (lat. \"Divide and conquer\") - by splitting a window "
		      "into two parts (e.g. Window -> <img width='16' height='16' src=\"%1\"> Split View "
		      "Left/Right) you can make Konqueror appear the way you like. You"
		      " can even load some example view-profiles (e.g. Midnight Commander)"
		      ", or create your own ones." , view_left_right_icon_path))
	  .arg( i18n( "Use the <a href=\"%1\">user-agent</a> feature if the website you are visiting "
                      "asks you to use a different browser "
		      "(and do not forget to send a complaint to the webmaster!)" , QString("exec:/kcmshell useragent")) )
	  .arg( i18n( "The <img width='16' height='16' src=\"%1\"> History in your SideBar ensures "
		      "that you can keep track of the pages you have visited recently.", history_icon_path) )
	  .arg( i18n( "Use a caching <a href=\"%1\">proxy</a> to speed up your"
		      " Internet connection.", QString("exec:/kcmshell proxy")) )
	  .arg( i18n( "Advanced users will appreciate the Konsole which you can embed into "
		      "Konqueror (Window -> <img width='16' height='16' SRC=\"%1\"> Show "
 		      "Terminal Emulator).", openterm_icon_path))
	  .arg( i18n( "Thanks to <a href=\"%1\">DCOP</a> you can have full control over Konqueror using a script."
, QString("exec:/kdcop")) )
	  .arg( i18n( "<img width='16' height='16' src=\"%1\">" ,  continue_icon_path ) )
	  .arg( i18n( "Next: Specifications" ) )
          ;


    s_tips_html = new QString( res );

    return res;
}


QString KonqAboutPageFactory::plugins()
{
    if ( s_plugins_html )
        return *s_plugins_html;

    QString res = loadFile( KStandardDirs::locate( "data", kapp->layoutDirection() == Qt::RightToLeft ? "konqueror/about/plugins_rtl.html" : "konqueror/about/plugins.html" ))
                  .arg(i18n("Installed Plugins"))
                  .arg(i18n("<td>Plugin</td><td>Description</td><td>File</td><td>Types</td>"))
                  .arg(i18n("Installed"))
                  .arg(i18n("<td>Mime Type</td><td>Description</td><td>Suffixes</td><td>Plugin</td>"));
    if ( res.isEmpty() )
	return res;

    s_plugins_html = new QString( res );

    return res;
}


KonqAboutPage::KonqAboutPage( //KonqMainWindow *
                              QWidget *parentWidget, QObject *parent )
    : KHTMLPart( parentWidget, parent, BrowserViewGUI )
{
    //m_mainWindow = mainWindow;
    QTextCodec* codec = KGlobal::locale()->codecForEncoding();
    if (codec)
	setCharset(codec->name(), true);
    else
	setCharset("iso-8859-1", true);
    // about:blah isn't a kioslave -> disable View source
    KAction * act = actionCollection()->action("viewDocumentSource");
    if ( act )
      act->setEnabled( false );
}

KonqAboutPage::~KonqAboutPage()
{
}

bool KonqAboutPage::openURL( const KUrl &u )
{
    if (u.url() == "about:plugins")
       serve( KonqAboutPageFactory::plugins(), "plugins" );
    else serve( KonqAboutPageFactory::launch(), "konqueror" );
    return true;
}

bool KonqAboutPage::openFile()
{
    return true;
}

void KonqAboutPage::saveState( QDataStream &stream )
{
    stream << m_htmlDoc;
    stream << m_what;
}

void KonqAboutPage::restoreState( QDataStream &stream )
{
    stream >> m_htmlDoc;
    stream >> m_what;
    serve( m_htmlDoc, m_what );
}

void KonqAboutPage::serve( const QString& html, const QString& what )
{
    m_what = what;
    begin( KUrl( QString("about:%1").arg(what) ) );
    write( html );
    end();
    m_htmlDoc = html;
}

void KonqAboutPage::urlSelected( const QString &url, int button, int state, const QString &target, KParts::URLArgs _args )
{
    KUrl u( url );
    if ( u.protocol() == "exec" )
    {
        QStringList args = url.mid( 6 ).split(QChar( ' ' ), QString::SkipEmptyParts );
        QString executable = args[ 0 ];
        args.erase( args.begin() );
        KToolInvocation::kdeinitExec( executable, args );
        return;
    }

    if ( url == QLatin1String("launch.html") )
    {
        emit browserExtension()->openUrlNotify();
	serve( KonqAboutPageFactory::launch(), "konqueror" );
        return;
    }
    else if ( url == QLatin1String("intro.html") )
    {
        emit browserExtension()->openUrlNotify();
        serve( KonqAboutPageFactory::intro(), "konqueror" );
        return;
    }
    else if ( url == QLatin1String("specs.html") )
    {
        emit browserExtension()->openUrlNotify();
	serve( KonqAboutPageFactory::specs(), "konqueror" );
        return;
    }
    else if ( url == QLatin1String("tips.html") )
    {
        emit browserExtension()->openUrlNotify();
        serve( KonqAboutPageFactory::tips(), "konqueror" );
        return;
    }

    else if ( url == QLatin1String("config:/disable_overview") )
    {
	if ( KMessageBox::questionYesNo( widget(),
					 i18n("Do you want to disable showing "
					      "the introduction in the webbrowsing profile?"),
					 i18n("Faster Startup?"),i18n("Disable"),i18n("Keep") )
	     == KMessageBox::Yes )
	{
	    QString profile = KStandardDirs::locateLocal("data", "konqueror/profiles/webbrowsing");
	    KSaveFile file( profile );
	    if ( file.status() == 0 ) {
		QByteArray content = "[Profile]\n"
			           "Name=Web-Browser";
		fputs( content.data(), file.fstream() );
		file.close();
	    }
	}
	return;
    }

    KHTMLPart::urlSelected( url, button, state, target, _args );
}

#include "konq_aboutpage.moc"
