#include <signal.h>

#include <qdir.h>
#include <kapp.h>
#include <qfileinf.h>
#include "kftypes.h"
#include "kfarch.h"
#include "kfind.h"
#include "kfsave.h"

KfSaveOptions *saving;

int main( int argc, char ** argv ) 
  {
    int i;
    QString searchPath;

    QFont default_font("Helvetica", 12);       
    QApplication::setFont(default_font );

    KApplication a( argc, argv, "kfind" );

    (void)signal(SIGCHLD, SIG_DFL);

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

    Kfind w(0L,0L,searchPath.data());

    a.setMainWidget( &w );
    w.show();
    return a.exec();
  };
 
