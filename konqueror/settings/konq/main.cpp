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

KConfig *g_pConfig;

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
  KHTTPOptions  *m_pHTTPOptions;
  KMiscOptions  *m_pMiscOptions;
  KBehaviourOptions *m_pBehaviourOptions;
};

KonqControlApplication::KonqControlApplication( int &argc, char **argv )
: KControlApplication( argc, argv, "kcmkonq" )
{
  m_pFontOptions  = 0L;
  m_pColorOptions = 0L;
  m_pHTTPOptions  = 0L;
  m_pMiscOptions  = 0L;

  if ( !runGUI() )
    return;

  if ( !pages || pages->contains( "behaviour" ) )
    addPage( m_pBehaviourOptions = new KBehaviourOptions( dialog, "behaviour" ), i18n( "&Behaviour" ), "konq-1.html" );

  if ( !pages || pages->contains( "font" ) )
    addPage( m_pFontOptions = new KFontOptions( dialog, "font" ), i18n( "&Font" ), "konq-2.html" );
   
  if ( !pages || pages->contains( "color" ) )
    addPage( m_pColorOptions = new KColorOptions( dialog, "color" ), i18n( "&Color" ), "konq-3.html" );
   
  if ( !pages || pages->contains( "http" ) )
    addPage( m_pHTTPOptions = new KHTTPOptions( dialog, "http" ), i18n( "&HTTP" ), "konq-4.html" );
    
  if ( !pages || pages->contains( "misc" ) )
    addPage( m_pMiscOptions = new KMiscOptions( dialog, "misc" ), i18n( "&Other" ), "konq-5.html" );
   
  if ( m_pFontOptions || m_pColorOptions || m_pHTTPOptions || m_pMiscOptions || m_pBehaviourOptions )
     dialog->show();
  else
  {
    fprintf(stderr, i18n("usage: %s [-init | {behavour,font,color,http,misc}]\n").ascii(), argv[0] );;
    justInit = true;
  }
}

void KonqControlApplication::init()
{
  if ( m_pBehaviourOptions )
    m_pBehaviourOptions->loadSettings();

  if ( m_pFontOptions )
    m_pFontOptions->loadSettings();
    
  if ( m_pColorOptions )
    m_pColorOptions->loadSettings();
    
  if ( m_pHTTPOptions )
    m_pHTTPOptions->loadSettings();
    
  if ( m_pMiscOptions )
    m_pMiscOptions->loadSettings();
}

void KonqControlApplication::defaultValues()
{
  if ( m_pBehaviourOptions )
    m_pBehaviourOptions->defaultSettings();

  if ( m_pFontOptions )
    m_pFontOptions->defaultSettings();
    
  if ( m_pColorOptions )
    m_pColorOptions->defaultSettings();
    
  if ( m_pHTTPOptions )
    m_pHTTPOptions->defaultSettings();
    
  if ( m_pMiscOptions )
    m_pMiscOptions->defaultSettings();
}

void KonqControlApplication::apply()
{
  if ( m_pBehaviourOptions )
    m_pBehaviourOptions->applySettings();

  if ( m_pFontOptions )
    m_pFontOptions->applySettings();
    
  if ( m_pColorOptions )
    m_pColorOptions->applySettings();

  if ( m_pHTTPOptions )
    m_pHTTPOptions->applySettings();    
 
  if ( m_pMiscOptions )
    m_pMiscOptions->applySettings();

  QString exeloc = locate("exe","kfmclient");
  if ( exeloc.isEmpty() )
    KMessageBox::error( 0L, i18n( "Can't find the kfmclient program - can't apply configuration dynamically" ), i18n( "Error" ) );
  else
    if ( fork() == 0 )
    {
      // execute 'kfmclient configure'
      execl(exeloc, "kfmclient", "configure", 0L);
      warning("Error launching 'kfmclient configure' !");
      exit(1);
    }
}

int main(int argc, char **argv )
{
  g_pConfig = new KConfig( "konquerorrc", false, false );
  KonqControlApplication app( argc, argv );
  
  app.setTitle( i18n( "Konqueror Configuration" ) );
  
  int ret = 0;
  if ( app.runGUI() )
    ret = app.exec();
  
  delete g_pConfig;
  return ret;
}
