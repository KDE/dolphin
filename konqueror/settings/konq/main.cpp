// (c) Torben Weis 1998
// (c) David Faure 1998

#include <stdio.h>

#include <kcontrol.h>
#include <klocale.h>

#include "htmlopts.h"
#include "khttpoptdlg.h"
#include "miscopts.h"

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

  if ( !pages || pages->contains( "font" ) )
    addPage( m_pFontOptions = new KFontOptions( dialog, "font" ), i18n( "&Font" ), "konq-1.html" );
   
  if ( !pages || pages->contains( "color" ) )
    addPage( m_pColorOptions = new KColorOptions( dialog, "color" ), i18n( "&Color" ), "konq-2.html" );
   
  if ( !pages || pages->contains( "http" ) )
    addPage( m_pHTTPOptions = new KHTTPOptions( dialog, "http" ), i18n( "&HTTP" ), "konq-3.html" );
    
  if ( !pages || pages->contains( "misc" ) )
    addPage( m_pMiscOptions = new KMiscOptions( dialog, "misc" ), i18n( "&Other" ), "konq-4.html" );
   
  if ( m_pFontOptions || m_pColorOptions || m_pHTTPOptions || m_pMiscOptions )
     dialog->show();
  else
  {
    fprintf(stderr, i18n("usage: %s [-init | {font,color,misc}]\n").ascii(), argv[0] );;
    justInit = true;
  }
}

void KonqControlApplication::init()
{
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
  if ( m_pFontOptions )
    m_pFontOptions->applySettings();
    
  if ( m_pColorOptions )
    m_pColorOptions->applySettings();

  if ( m_pHTTPOptions )
    m_pHTTPOptions->applySettings();    
 
  if ( m_pMiscOptions )
    m_pMiscOptions->applySettings();
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
