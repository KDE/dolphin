// (c) Torben Weis 1998
// (c) David Faure 1998

#include <config.h>
#include <stdio.h>

#include <kcontrol.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "htmlopts.h"
#include "khttpoptdlg.h"
#include "miscopts.h"
#include "behaviour.h"
#include "rootopts.h"

KConfig *g_pConfig = 0;

class KonqControlApplication: public KControlApplication
{
public:
  KonqControlApplication( int &argc, char **argv );

  virtual void init();
  virtual void apply();
  virtual void defaultValues();

private:
  KFontOptions  *m_pFontOptions;
  KColorOptions *m_pColorOptions;
  KHtmlOptions *m_pHtmlOptions;
  KHTTPOptions  *m_pHTTPOptions;
  KMiscOptions  *m_pMiscOptions;
  KBehaviourOptions *m_pBehaviourOptions;
  KRootOptions *m_pRootOptions;
};

KonqControlApplication::KonqControlApplication( int &argc, char **argv )
: KControlApplication( argc, argv, "kcmkonq" )
{
  m_pBehaviourOptions  = 0L;
  m_pFontOptions  = 0L;
  m_pColorOptions = 0L;
  m_pHtmlOptions = 0L;
  m_pHTTPOptions  = 0L;
  m_pMiscOptions  = 0L;
  m_pRootOptions  = 0L;

  bool _desktop = argc > 1 && !strcmp(argv[1], "desktop");
  g_pConfig = new KConfig( _desktop ? "kdesktoprc" : "konquerorrc", false, false );

  if ( _desktop )
      setTitle( i18n( "Desktop Icons Configuration" ) );
  else
      setTitle( i18n( "Konqueror Configuration" ) );

  if ( !runGUI() )
    return;

  QString groupname = "HTML Settings"; // default group

  // Only do this if called explicitely with the "desktop" argument
  if ( ( !pages || pages->contains( "desktop" ) ) && ( argc > 1 ) )
  {
   addPage( m_pRootOptions = new KRootOptions( dialog, "desktop" ), i18n( "&Desktop Icons" ), "kdesktop-1.html" );
   groupname = "Desktop Settings"; // group for kdesktop
  }

  if ( !pages || pages->contains( "behaviour" ) )
    addPage( m_pBehaviourOptions = new KBehaviourOptions( dialog, "behaviour" ), i18n( "&Behaviour" ), "konq-1.html" );

  if ( !pages || pages->contains( "font" ) )
    addPage( m_pFontOptions = new KFontOptions( dialog, "font", groupname ), i18n( "&Font" ), "konq-2.html" );

  if ( !pages || pages->contains( "color" ) )
    addPage( m_pColorOptions = new KColorOptions( dialog, "color", groupname ), i18n( "&Color" ), "konq-3.html" );

  // ### FIXME: get the ref to the help page right
  if ( !pages || pages->contains( "html" ) )
    addPage( m_pHtmlOptions = new KHtmlOptions( dialog, "html"), i18n( "&Html" ), "konq-3.html" );

  if ( !pages || pages->contains( "http" ) )
    addPage( m_pHTTPOptions = new KHTTPOptions( dialog, "http" ), i18n( "H&TTP" ), "konq-4.html" );

  if ( !pages || pages->contains( "misc" ) )
    addPage( m_pMiscOptions = new KMiscOptions( dialog, "misc" ), i18n( "&Other" ), "konq-5.html" );

  if ( m_pRootOptions || m_pFontOptions || m_pColorOptions || m_pHTTPOptions || m_pMiscOptions || m_pBehaviourOptions )
     dialog->show();
  else
  {
    fprintf(stderr, i18n("usage: %s [-init | {behavour,font,color,http,misc,root}]\n").ascii(), argv[0] );;
    justInit = true;
  }
}

void KonqControlApplication::init()
{
  if ( m_pRootOptions )
    m_pRootOptions->loadSettings();

  if ( m_pBehaviourOptions )
    m_pBehaviourOptions->loadSettings();

  if ( m_pFontOptions )
    m_pFontOptions->loadSettings();

  if ( m_pColorOptions )
    m_pColorOptions->loadSettings();

  if ( m_pHtmlOptions )
    m_pHtmlOptions->loadSettings();

  if ( m_pHTTPOptions )
    m_pHTTPOptions->loadSettings();

  if ( m_pMiscOptions )
    m_pMiscOptions->loadSettings();
}

void KonqControlApplication::defaultValues()
{
  if ( m_pRootOptions )
    m_pRootOptions->defaultSettings();

  if ( m_pBehaviourOptions )
    m_pBehaviourOptions->defaultSettings();

  if ( m_pFontOptions )
    m_pFontOptions->defaultSettings();

  if ( m_pColorOptions )
    m_pColorOptions->defaultSettings();

  if ( m_pHtmlOptions )
    m_pHtmlOptions->defaultSettings();

  if ( m_pHTTPOptions )
    m_pHTTPOptions->defaultSettings();

  if ( m_pMiscOptions )
    m_pMiscOptions->defaultSettings();
}

void KonqControlApplication::apply()
{
  if ( m_pRootOptions )
    m_pRootOptions->applySettings();

  if ( m_pBehaviourOptions )
    m_pBehaviourOptions->applySettings();

  if ( m_pFontOptions )
    m_pFontOptions->applySettings();

  if ( m_pColorOptions )
    m_pColorOptions->applySettings();

  if ( m_pHtmlOptions )
    m_pHtmlOptions->applySettings();

  if ( m_pHTTPOptions )
    m_pHTTPOptions->applySettings();

  if ( m_pMiscOptions )
    m_pMiscOptions->applySettings();

  QString exeloc = locate("exe","kfmclient");
  if ( exeloc.isEmpty() ) {
  	  KMessageBox::error( 0L,
	  i18n( "Can't find the kfmclient program - can't apply configuration dynamically" ), i18n( "Error" ) );
	return;
  }

  QApplication::flushX();

  if ( fork() == 0 )
  {
	  // execute 'kfmclient configure' or 'kfmclient configureDesktop'
	  if ( m_pRootOptions )
	  {
		  execl(exeloc, "kfmclient", "configureDesktop", 0L);
		  warning("Error launching 'kfmclient configureDesktop' !");
	  } else
	  {
		  execl(exeloc, "kfmclient", "configure", 0L);
		  warning("Error launching 'kfmclient configure' !");
	  }
	  exit(1);
  }
}

int main(int argc, char **argv )
{
    KonqControlApplication app( argc, argv );


  int ret = 0;
  if ( app.runGUI() )
    ret = app.exec();

  delete g_pConfig;
  return ret;
}
