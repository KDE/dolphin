#include <signal.h>

#include <qdir.h>
#include <kapp.h>
#include <kiconloader.h>
#include <qfileinfo.h>
#include <qfile.h>
#include "kftypes.h"
#include "kfarch.h"
#include "kfindtop.h"
#include "kfsave.h"
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <version.h>

static const char *description = 
	I18N_NOOP("KDE File find utility.");

static KCmdLineOptions options[] =
{
  { "+[searchpath]", "Path(s) to search.", 0 },
  { 0,0,0 }
};

KfSaveOptions *saving;

int main( int argc, char ** argv ) 
{
    KCmdLineArgs::init(argc, argv, "kfind", description, KFIND_VERSION);

    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    //Scan for saving options in kfind resource file
    saving = new KfSaveOptions();

    // Scan for available filetypes
    KfFileType::init();   

    // Scan for avaiable archivers in kfind resource file
    KfArchiver::init();

    KCmdLineArgs *args= KCmdLineArgs::parsedArgs();

    QString searchPath;
    if (args->count() > 0)
       searchPath = QFile::decodeName(args->arg(0));
    else
       searchPath = QDir::currentDirPath();

    args->clear();

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
      while (KTMainWindow::canBeRestored(n)){
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
    kfind->nameSetFocus();
    int ret =  app.exec();

    return ret;
};
 
