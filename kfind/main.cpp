#include <QDir>
#include <QFile>

#include <kapplication.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kopenwith.h>

#include "kfinddlg.h"
#include "version.h"

static const char description[] = I18N_NOOP("KDE file find utility");

static KCmdLineOptions options[] =
{
  { "+[searchpath]", I18N_NOOP("Path(s) to search"), 0 },
  KCmdLineLastOption
};

int main( int argc, char ** argv )
{
  KLocale::setMainCatalog("kfindpart");
  KAboutData aboutData( "kfind", I18N_NOOP("KFind"),
      KFIND_VERSION, description, KAboutData::License_GPL,
      I18N_NOOP("(c) 1998-2003, The KDE Developers"));

  aboutData.addAuthor("Eric Coquelle", I18N_NOOP("Current Maintainer"), "coquelle@caramail.com");
  aboutData.addAuthor("Mark W. Webb", I18N_NOOP("Developer"), "markwebb@adelphia.net");
  aboutData.addAuthor("Beppe Grimaldi", I18N_NOOP("UI Design & more search options"), "grimalkin@ciaoweb.it");
  aboutData.addAuthor("Martin Hartig");
  aboutData.addAuthor("Stephan Kulow", 0, "coolo@kde.org");
  aboutData.addAuthor("Mario Weilguni",0, "mweilguni@sime.com");
  aboutData.addAuthor("Alex Zepeda",0, "zipzippy@sonic.net");
  aboutData.addAuthor("Miroslav Fl�r",0, "flidr@kky.zcu.cz");
  aboutData.addAuthor("Harri Porten",0, "porten@kde.org");
  aboutData.addAuthor("Dima Rogozin",0, "dima@mercury.co.il");
  aboutData.addAuthor("Carsten Pfeiffer",0, "pfeiffer@kde.org");
  aboutData.addAuthor("Hans Petter Bieker", 0, "bieker@kde.org");
  aboutData.addAuthor("Waldo Bastian", I18N_NOOP("UI Design"), "bastian@kde.org");
  aboutData.addAuthor("Alexander Neundorf", 0, "neundorf@kde.org");
  aboutData.addAuthor("Clarence Dang", 0, "dang@kde.org");

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;

  KCmdLineArgs *args= KCmdLineArgs::parsedArgs();

  KUrl url;
  if (args->count() > 0)
    url = args->url(0);
  if (url.isEmpty())
    url = QDir::currentPath();
  if (url.isEmpty())
    url = QDir::homePath();
  args->clear();

  KfindDlg kfinddlg(url);
  return kfinddlg.exec();
}
