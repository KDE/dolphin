// (c) Torben Weis 1998
// (c) David Faure 1998

#include <stdio.h>

#include <kcontrol.h>
#include <klocale.h>

#include "kproxydlg.h"
#include "ksmboptdlg.h"
#include "useragentdlg.h"
#include "kcookiesdlg.h"
#include "kprofiledlg.h"

KConfig *g_pConfig = 0L;

class KIOControlApplication: public KControlApplication
{
public:
  KIOControlApplication( int &argc, char **argv );

  virtual void init();
  virtual void apply();
  virtual void defaultValues();

private:
  KProxyOptions    *m_pProxyOptions;
  KSMBOptions      *m_pSMBOptions;
  UserAgentOptions *m_pUserAgentOptions;
  KCookiesOptions  *m_pCookiesOptions;
  KProfileOptions  *m_pProfileOptions;
};

KIOControlApplication::KIOControlApplication( int &argc, char **argv )
: KControlApplication( argc, argv, "kcmkio" )
{
  m_pProxyOptions     = 0L;
  m_pSMBOptions       = 0L;
  m_pUserAgentOptions = 0L;
  m_pCookiesOptions   = 0L;
  m_pProfileOptions   = 0L;
  
  if ( !runGUI() )
    return;

  if ( !pages || pages->contains( "proxy" ) )
   addPage( m_pProxyOptions = new KProxyOptions( dialog, "proxy" ), i18n( "&Proxy" ), "kio-1.html" );
     
  if ( !pages || pages->contains( "smb" ) )
   addPage( m_pSMBOptions = new KSMBOptions( dialog, "smb" ), i18n( "&SMB" ), "kio-3.html" );
   
  if ( !pages || pages->contains( "useragent" ) )
   addPage( m_pUserAgentOptions = new UserAgentOptions( dialog, "useragent" ), i18n( "User &Agent" ), "kio-4.html" );
   
  if ( !pages || pages->contains( "cookies" ) )
   addPage( m_pCookiesOptions = new KCookiesOptions( dialog, "cookies" ), i18n( "Coo&kies" ), "kio-5.html" );

  if ( !pages || pages->contains( "profile" ) )
   addPage( m_pProfileOptions = new KProfileOptions( argc, argv, dialog, "profile" ), i18n( "&User Preferences For Services" ), "kio-6.html" );

  if ( m_pProxyOptions || m_pSMBOptions || m_pUserAgentOptions || 
       m_pCookiesOptions || m_pProfileOptions )
     dialog->show();
  else
  {
    fprintf(stderr, i18n("usage: %s [-init | {proxy,smb,useragent,cookies,profile}]\n").ascii(), argv[0] );;
    justInit = true;
  }
}

void KIOControlApplication::init()
{
  if ( m_pProxyOptions )
    m_pProxyOptions->loadSettings();
    
  if ( m_pSMBOptions )
    m_pSMBOptions->loadSettings();
    
  if ( m_pUserAgentOptions )
    m_pUserAgentOptions->loadSettings();
    
  if ( m_pCookiesOptions )
    m_pCookiesOptions->loadSettings();
    
  if ( m_pProfileOptions )
    m_pProfileOptions->loadSettings();
}

void KIOControlApplication::defaultValues()
{
  if ( m_pProxyOptions )
    m_pProxyOptions->defaultSettings();
    
  if ( m_pSMBOptions )
    m_pSMBOptions->defaultSettings();
    
  if ( m_pUserAgentOptions )
    m_pUserAgentOptions->defaultSettings();
    
  if ( m_pCookiesOptions )
    m_pCookiesOptions->defaultSettings();

  if ( m_pProfileOptions )
    m_pProfileOptions->defaultSettings();
}

void KIOControlApplication::apply()
{
  if ( m_pProxyOptions )
    m_pProxyOptions->applySettings();
    
  if ( m_pSMBOptions )
    m_pSMBOptions->applySettings();
    
  if ( m_pUserAgentOptions )
    m_pUserAgentOptions->applySettings();
    
  if ( m_pCookiesOptions )
    m_pCookiesOptions->applySettings();

  if ( m_pProfileOptions )
    m_pProfileOptions->applySettings();
}

int main(int argc, char **argv )
{
  g_pConfig = new KConfig( "kioslaverc", false, false );
  KIOControlApplication app( argc, argv );
  
  app.setTitle( i18n( "KDE Network Configuration" ) );
  
  int ret = 0;
  if ( app.runGUI() )
    ret = app.exec();
  
  delete g_pConfig;
  return ret;
}
