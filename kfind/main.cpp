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
#include <kaboutdata.h>
#include <kdebug.h>

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
	KAboutData aboutData( "kfind", I18N_NOOP("KFind"),
		KFIND_VERSION, description, KAboutData::License_GPL,
		"(c) 1998-2000, The KDE Developers");
	aboutData.addAuthor("Martin Hartig");
	aboutData.addAuthor("Mario Weilguni",0, "mweilguni@sime.com");
	aboutData.addAuthor("Alex Zepeda",0, "jazepeda@pacbell.net");
	aboutData.addAuthor("Miroslav Flídr",0, "flidr@kky.zcu.cz");
	aboutData.addAuthor("Harri Porten",0, "porten@kde.org");
	aboutData.addAuthor("Dima Rogozin",0, "dima@mercury.co.il");
	aboutData.addAuthor("Carsten Pfeiffer",0, "pfeiffer@kde.org");
	
	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

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

/*
    // session management (Matthias)
    if (kapp->isRestored()){
      int n = 1;
      while (KTMainWindow::canBeRestored(n)){
	kfind = new KfindTop(searchPath);
	kfind->restore(n);
	n++;
      }
      // end session management
    } else {
*/
      kfind = new KfindTop(searchPath);
      kfind->show();
//    }
    app.setMainWidget(kfind);
    kfind->nameSetFocus();
    int ret =  app.exec();

    return ret;
};

