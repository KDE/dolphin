#include <signal.h>

#include <qdir.h>
#include <kapp.h>
#include <qfileinf.h>
#include "kftypes.h"
#include "kfarch.h"
#include "kfindtop.h"
#include "kfsave.h"

KfSaveOptions *saving;

int main( int argc, char ** argv ) 
  {
    int i;
    QString searchPath;

    KApplication *app = new KApplication( argc, argv, "kfind" );

//      QFont default_font("-*-helvetica-medium-r-*-*-*-*-*-*-*-*-iso8859-2");
//      default_font.setRawMode( TRUE );
//      QApplication::setFont(default_font );
     

// Kalle Dalheimer, 31.08.97: no longer needed, now handled by KProcess
//    (void)signal(SIGCHLD, SIG_DFL);

     //Scan for saving options in kfind resource file
     saving = new KfSaveOptions();

     // Scan for available filetypes
     KfFileType::init();   

     // Scan for avaiable archivers in kfind resource file
     KfArchiver::init();
 
    
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

     if ( strchr(searchPath.data(),':') )
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

    KfindTop *kfind= new KfindTop(searchPath); 

    app->setMainWidget( kfind );
    kfind->show();

    int ret =  app->exec();
    
    delete kfind;
    delete app;

    return ret;
  };
 
