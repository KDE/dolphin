#include <signal.h>

#include <qdir.h>
#include <kapp.h>
#include <kiconloader.h>
#include <qfileinfo.h>
#include "kftypes.h"
#include "kfarch.h"
#include "kfindtop.h"
#include "kfsave.h"
#include <kglobal.h>
#include <kded_instance.h>
#include <ktrader.h>
#include <kregistry.h>
#include <kregfactories.h>

KfSaveOptions *saving;

int main( int argc, char ** argv ) 
  {
    int i;
    QString searchPath;

    KApplication app( argc, argv, "kfind" );
  
    //Scan for saving options in kfind resource file
    saving = new KfSaveOptions();

    // Scan for available filetypes
    KfFileType::init();   

    // Scan for avaiable archivers in kfind resource file
    KfArchiver::init();
 
    CORBA::ORB_ptr orb = CORBA::ORB_init( argc, argv, "mico-local-orb" );
    KdedInstance kded( argc, argv, orb );
    
    KTraderServiceProvider sp;
    
    KRegistry::self()->addFactory( new KServiceTypeFactory );
    KRegistry::self()->load();
    
    for( i=1; i<argc; i++)
      {
 	if (argv[i][0]=='-')
 	  continue;
	
 	searchPath = argv[i];
 	break;
      };

    if (i==argc) 
      searchPath = QDir::currentDirPath();

    if ( searchPath.isNull() )
      searchPath = getenv( "HOME" );

    if (searchPath.contains(':') > 0)
      {
 	if (searchPath.left(searchPath.find(":"))=="file")
 	  searchPath.remove(0,5);
 	else
 	  searchPath = getenv( "HOME" );
      };
    
    QFileInfo filename(searchPath);
    if ( filename.exists() )
      {
 	if ( filename.isDir() )
 	  searchPath = filename.filePath();
 	else
 	  searchPath = filename.dirPath(TRUE);
      }
    else
      searchPath = getenv( "HOME" );

    KfindTop *kfind= NULL;

    // session management (Matthias)
    if (kapp->isRestored()){
      int n = 1;
      while (KTopLevelWidget::canBeRestored(n)){
	kfind = new KfindTop(searchPath.ascii()); 
	kfind->restore(n);
	n++;
      }
      // end session management
    } else {
      kfind = new KfindTop(searchPath.ascii());
      kfind->show();
    }
    app.setMainWidget(kfind);
    int ret =  app.exec();

    delete kfind;
    return ret;
  };
 
