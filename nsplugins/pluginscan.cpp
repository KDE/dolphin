/*

  This application scans for Netscape plugins and create a cache and
  the necessary mimelnk and service files.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <unistd.h>
              
#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qbuffer.h>

#include <dcopclient.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klibloader.h>
#include <kconfig.h>
#include <kdesktopfile.h>
#include <kservicetype.h>
#include <kmimetype.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>

#include "sdk/npupp.h"


KConfig *infoConfig = 0;


bool isPluginMimeType( QString fname )
{
    KDesktopFile cfg( fname, true );
    cfg.setDesktopGroup();
    return cfg.hasKey( "X-KDE-nsplugin" );
}


void deletePluginMimeTypes()
{
    // iterate through local mime type directories
    QString dir = KGlobal::dirs()->saveLocation( "mime" );
    kdDebug(1433) << "Removing nsplugin MIME types in " << dir << endl;
    QDir dirs( dir, QString::null, QDir::Name|QDir::IgnoreCase, QDir::Dirs );
    if ( !dirs.exists() ) {
        kdDebug(1433) << "Directory not found" << endl;
        return;
    }

    for ( unsigned int i=0; i<dirs.count(); i++ ) {
        if ( !dirs[i].contains(".") ) {

            // check all mime types for X-KDE-nsplugin flag
            kdDebug(1433) << " - Looking in " << dirs[i] << endl;
            QDir files( dirs.absFilePath(dirs[i]), QString::null,
                        QDir::Name|QDir::IgnoreCase, QDir::Files );
            if ( files.exists( dir ) ) {

                for (unsigned int i=0; i<files.count(); i++) {

                    // check .desktop file
                    kdDebug(1433) << "   - Checking " << files[i] << endl;
                    if ( isPluginMimeType(files.absFilePath(files[i])) ) {
                        kdDebug(1433) << "     - Removing " << files[i] << endl;
                        files.remove( files[i] );
                    }

                }
            }

        }
    }
}


void generateMimeType( QString mime, QString extensions, QString pluginName, QString description )
{
    kdDebug(1433) << "-> generateMimeType mime=" << mime << " ext="<< extensions << endl;

    // get directory from mime string
    QString dir;
    QString name;
    int pos = mime.findRev('/');
    if ( pos<0 ) {
        kdDebug(1433) << "Invalid MIME type " << mime << endl;
        kdDebug(1433) << "<- generateMimeType" << endl;
        return;
    }
    dir = KGlobal::dirs()->saveLocation( "mime", mime.left(pos) );
    name = mime.mid(pos);

    // create mimelnk file
    QFile f( dir + name + ".desktop" );
    if ( f.open(IO_WriteOnly) ) {

        // write .desktop file
        QTextStream ts(&f);

        ts << "[Desktop Entry]" << endl;
        ts << "Type=MimeType" << endl;
        ts << "MimeType=" << mime << endl;
        ts << "Icon=netscape_doc" << endl;
        ts << "Comment=Netscape " << pluginName << endl;
        ts << "X-KDE-AutoEmbed=true" << endl;
        ts << "X-KDE-nsplugin=true" << endl;
        ts << "InitialPreference=0" << endl;

        if (!extensions.isEmpty()) {
            QStringList exts = QStringList::split(",", extensions);
            QStringList patterns;
            for (QStringList::Iterator it=exts.begin(); it != exts.end(); ++it)
                patterns.append( "*." + (*it).stripWhiteSpace() );

            ts << "Patterns=" << patterns.join( ";" ) << endl;
        }

        if (!description.isEmpty())
            ts << "Name=" << description << endl;
        else
            ts << "Name=" << i18n("Netscape plugin mimeinfo") << endl;

        f.close();
    }

    kdDebug(1433) << "<- generateMimeType" << endl;
}


void registerPlugin( const QString &name, const QString &description,
                     const QString &file, const QString &mimeInfo )
{
    // global stuff
    infoConfig->setGroup( QString::null );
    int num = infoConfig->readNumEntry( "number", 0 );
    infoConfig->writeEntry( "number", num+1 );

    // create plugin info
    infoConfig->setGroup( QString::number(num) );
    infoConfig->writeEntry( "name", name );
    infoConfig->writeEntry( "description", description );
    infoConfig->writeEntry( "file", file );
    infoConfig->writeEntry( "mime", mimeInfo );
}

int tryCheck(int write_fd, const QString &absFile)
{
    KLibrary *_handle = KLibLoader::self()->library( QFile::encodeName(absFile) );
    if (!_handle) {
        kdDebug(1433) << " - open failed, skipping " << endl;
        return 1;
    }

    // ask for name and description
    QString name = i18n("Unnamed plugin");
    QString description;

    NPError (*func_GetValue)(void *, NPPVariable, void *) =
        (NPError(*)(void *, NPPVariable, void *))
        _handle->symbol("NP_GetValue");
    if ( func_GetValue ) {

        // get name
        char *buf = 0;
        NPError err = func_GetValue( 0, NPPVpluginNameString,
                                     (void*)&buf );
        if ( err==NPERR_NO_ERROR )
            name = QString::fromLatin1( buf );
        kdDebug() << "name = " << name << endl;

        // get name
        NPError nperr = func_GetValue( 0, NPPVpluginDescriptionString,
                                     (void*)&buf );
        if ( nperr==NPERR_NO_ERROR )
            description = QString::fromLatin1( buf );
        kdDebug() << "description = " << description << endl;
    }
    else
        kdWarning() << "Plugin doesn't implement NP_GetValue"  << endl;

    // get mime description function pointer
    char* (*func_GetMIMEDescription)() =
        (char *(*)())_handle->symbol("NP_GetMIMEDescription");
    if ( !func_GetMIMEDescription ) {
        kdDebug(1433) << " - no GetMIMEDescription, skipping" << endl;
        KLibLoader::self()->unloadLibrary( QFile::encodeName(absFile) );
        return 1;
    }

    // ask for mime information
    QString mimeInfo = func_GetMIMEDescription();
    if ( mimeInfo.isEmpty() ) {
        kdDebug(1433) << " - no mime info returned, skipping" << endl;
        KLibLoader::self()->unloadLibrary( QFile::encodeName(absFile) );
        return 1;
    }

    // remove version info, as it is not used at the moment
    QRegExp versionRegExp(";version=[^:]*:");
    mimeInfo.replace( versionRegExp, ":");
    
    // unload plugin lib
    kdDebug(1433) << " - unloading plugin" << endl;
    KLibLoader::self()->unloadLibrary( QFile::encodeName(absFile) );

    // create a QDataStream for our IPC pipe (to send plugin info back to the parent)
    FILE *write_pipe = fdopen(write_fd, "w");
    QFile stream_file;
    stream_file.open(IO_WriteOnly, write_pipe);
    QDataStream stream(&stream_file);

    // return the gathered info to the parent
    stream << name;
    stream << description;
    stream << mimeInfo;
    
    return 0;
}

void scanDirectory( QString dir, QStringList &mimeInfoList,
                    QTextStream &cache )
{
    kdDebug(1433) << "-> scanDirectory dir=" << dir << endl;

    // iterate over all files
    QDir files( dir, QString::null, QDir::Name|QDir::IgnoreCase, QDir::Files );
    if ( !files.exists( dir ) ) {
        kdDebug(1433) << "No files found" << endl;
        kdDebug(1433) << "<- scanDirectory dir=" << dir << endl;
        return;
    }

    for (unsigned int i=0; i<files.count(); i++) {

        // ignore crashing libs
        if ( files[i]=="librvplayer.so" ||      // RealPlayer 5
             files[i]=="libnullplugin.so" ||    // Netscape Default Plugin
             files[i]=="cult3dplugin.so" ||     // Cult 3d plugin
             files[i].right(4) == ".jar" ||     // Java archive
             files[i].right(6) == ".class"        // Java class
            )     
            continue;

        // get absolute file path
        QString absFile = files.absFilePath( files[i] );
        kdDebug(1433) << "Checking library " << absFile << endl;

        // open the library and ask for the mimetype
        kdDebug(1433) << " - opening " << absFile << endl;
        
        cache.device()->flush();
        // fork, so that a crash in the plugin won't stop the scanning of other plugins
        int pipes[2];
        if (pipe(pipes) != 0) continue;
        int loader_pid = fork();

        if (loader_pid == -1) {

            // unable to fork
            continue;

        } else if (loader_pid == 0) {

           // inside the child
           close(pipes[0]);
           _exit(tryCheck(pipes[1], absFile));

        } else {

           close(pipes[1]);

           QBuffer m_buffer;
           m_buffer.open(IO_WriteOnly);
    	
           FILE *read_pipe = fdopen(pipes[0], "r");
           QFile q_read_pipe;
           q_read_pipe.open(IO_ReadOnly, read_pipe);
           
           char *data = (char *)malloc(4096);
           if (!data) continue;

           // when the child closes, we'll get an EOF (size == 0)
           while (int size = q_read_pipe.readBlock(data, 4096)) {
               if (size > 0)
                   m_buffer.writeBlock(data, size);
           }
           free(data);
           
           close(pipes[0]); // we no longer need the pipe's reading end
           
           // close the buffer and open for reading (from the start)
           m_buffer.close();
           m_buffer.open(IO_ReadOnly);
           
           // create a QDataStream for our buffer
           QDataStream stream(&m_buffer);
           
           if (stream.atEnd()) continue;

           QString name, description, mimeInfo;
           stream >> name;
           stream >> description;
           stream >> mimeInfo;

           // note the plugin name
           cache << "[" << absFile << "]" << endl;

           // get mime types from string
           QStringList types = QStringList::split( ';', mimeInfo );
           QStringList::Iterator type;
           for ( type=types.begin(); type!=types.end(); ++type ) {

              kdDebug(1433) << " - type=" << *type << endl;

              // write into type cache
              QStringList tokens = QStringList::split(':', *type, TRUE);
              QStringList::Iterator token;
              token = tokens.begin();
              cache << (*token).lower();
              ++token;
              for ( ; token!=tokens.end(); ++token )
                  cache << ":" << *token;
              cache << endl;

              // append type to MIME type list
              if ( !mimeInfoList.contains( *type ) )
                  mimeInfoList.append( name + ":" + *type );
           }

           // register plugin for javascript
           registerPlugin( name, description, files[i], mimeInfo );

        }
    }

    // iterate over all sub directories
    // NOTE: Mozilla doesn't iterate over subdirectories of the plugin dir.
    // We still do (as Netscape 4 did).
    QDir dirs( dir, QString::null, QDir::Name|QDir::IgnoreCase, QDir::Dirs );
    if ( !dirs.exists() )
      return;

    static int depth = 0; // avoid recursion because of symlink circles
    depth++;
    for ( unsigned int i=0; i<dirs.count(); i++ ) {
        if ( depth<8 && !dirs[i].contains(".") )
            scanDirectory( dirs.absFilePath(dirs[i]), mimeInfoList, cache );
    }
    depth--;

    kdDebug() << "<- scanDirectory dir=" << dir << endl;
}


QStringList getSearchPaths()
{
    QStringList searchPaths;

    KConfig *config = new KConfig("kcmnspluginrc", false);
    config->setGroup("Misc");

    // setup default paths
    if ( !config->hasKey("scanPaths") ) {
        QStringList paths;
        paths.append("$HOME/.netscape/plugins");
        paths.append("/usr/local/netscape/plugins");
        paths.append("/opt/netscape/plugins");
        paths.append("/opt/netscape/communicator/plugins");
        paths.append("/usr/lib/netscape/plugins");
        paths.append("/usr/lib/netscape/plugins-libc5");
        paths.append("/usr/lib/netscape/plugins-libc6");
        paths.append("/usr/lib/mozilla/plugins");
        paths.append("$MOZILLA_HOME/plugins");
        config->writeEntry( "scanPaths", paths );
    }

    // read paths
    config->setDollarExpansion( true );
    searchPaths = config->readListEntry( "scanPaths" );
    delete config;

    // append environment variable NPX_PLUGIN_PATH
    QStringList envs = QStringList::split(':', getenv("NPX_PLUGIN_PATH"));
    QStringList::Iterator it;
    for (it = envs.begin(); it != envs.end(); ++it)
        searchPaths.append(*it);

    return searchPaths;
}


void writeServicesFile( QStringList mimeTypes )
{
    QString fname = KGlobal::dirs()->saveLocation("services", "")
                    + "/nsplugin.desktop";
    kdDebug(1433) << "Creating services file " << fname << endl;

    QFile f(fname);
    if ( f.open(IO_WriteOnly) ) {

        QTextStream ts(&f);

        ts << "[Desktop Entry]" << endl;
        ts << "Name=" << i18n("Netscape plugin viewer") << endl;
        ts << "Type=Service" << endl;
        ts << "Icon=netscape" << endl;
        ts << "Comment=" << i18n("Netscape plugin viewer") << endl;
        ts << "X-KDE-Library=libnsplugin" << endl;
        ts << "InitialPreference=0" << endl;
        ts << "ServiceTypes=KParts/ReadOnlyPart,Browser/View" << endl;

        if (mimeTypes.count() > 0)
            ts << "MimeType=" << mimeTypes.join(";") << endl;

        f.close();
    } else
        kdDebug(1433) << "Failed to open file " << fname << endl;
}


void removeExistingExtensions( QString &extension )
{
    QStringList filtered;
    QStringList exts = QStringList::split( ",", extension );
    for ( QStringList::Iterator it=exts.begin(); it!=exts.end(); ++it ) {
        QString ext = (*it).stripWhiteSpace();

        KMimeType::Ptr mime = KMimeType::findByURL( KURL("file:///foo."+ext ),
                                                    0, true, true );
        if( mime->name()=="application/octet-stream" ||
            mime->comment().left(8)=="Netscape" ) {
            kdDebug() << "accepted" << endl;
            filtered.append( ext );
        }
    }

    extension = filtered.join( "," );
}


int main( int argc, char **argv )
{
    printf("10\n"); fflush(stdout);

    KAboutData aboutData( "nspluginscan", I18N_NOOP("nspluginscan"),
                          "0.3", "nspluginscan", KAboutData::License_GPL,
                          "(c) 2000,2001 by Stefan Schimanski" );

    KLocale::setMainCatalogue("nsplugin");
    KCmdLineArgs::init( argc, argv, &aboutData );
    KApplication app;

    // set up the paths used to look for plugins
    QStringList searchPaths = getSearchPaths();
    QStringList mimeInfoList;

    infoConfig = new KConfig( KGlobal::dirs()->saveLocation("data", "nsplugins") +
                              "/pluginsinfo" );
    infoConfig->writeEntry( "number", 0 );

    // open the cache file for the mime information
    QString cacheName = KGlobal::dirs()->saveLocation("data", "nsplugins")+"/cache";
    kdDebug(1433) << "Creating MIME cache file " << cacheName << endl;
    QFile cachef(cacheName);
    if (!cachef.open(IO_WriteOnly))
        return -1;
    QTextStream cache(&cachef);
    printf("20\n"); fflush(stdout);

    // read in the plugins mime information
    kdDebug(1433) << "Scanning directories" << endl;
    int count = searchPaths.count();
    int i = 0;
    for ( QStringList::Iterator it = searchPaths.begin();
          it != searchPaths.end(); ++it, ++i)
    {
        scanDirectory( *it, mimeInfoList, cache );
        printf("%d\n", 25 + (50*i) / count ); fflush(stdout);
    }

    printf("75\n"); fflush(stdout);
    // delete old mime types
    kdDebug(1433) << "Removing old mimetypes" << endl;
    deletePluginMimeTypes();
    printf("80\n");  fflush(stdout);

    // write mimetype files
    kdDebug(1433) << "Creating MIME type descriptions" << endl;
    QStringList mimeTypes;
    for ( QStringList::Iterator it=mimeInfoList.begin();
          it!=mimeInfoList.end(); ++it) {

      kdDebug(1433) << "Handling MIME type " << *it << endl;

      QStringList info = QStringList::split(":", *it, true);
      if ( info.count()==4 ) {
          QString pluginName = info[0];
          QString type = info[1].lower();
          QString extension = info[2];
          QString desc = info[3];

          // append to global mime type list
          if ( !mimeTypes.contains(type) ) {
              kdDebug(1433) << " - mimeType=" << type << endl;
              mimeTypes.append( type );
          }

          // check mimelnk file
          QString fname = KGlobal::dirs()->findResource("mime", type+".desktop");
          if ( fname.isEmpty() || isPluginMimeType(fname) ) {
              kdDebug(1433) << " - creating MIME type description" << endl;
              removeExistingExtensions( extension );
              generateMimeType( type, extension, pluginName, desc );
          } else
              kdDebug(1433) << " - already existant" << endl;
        }
    }
    printf("85\n"); fflush(stdout);

    // close files
    kdDebug(1433) << "Closing cache file" << endl;
    cachef.close();

    infoConfig->sync();
    delete infoConfig;

    // write plugin lib service file
    writeServicesFile( mimeTypes );
    printf("90\n"); fflush(stdout);

    DCOPClient *dcc = kapp->dcopClient();
    if ( !dcc->isAttached() )
        dcc->attach();
    // Tel kded to update sycoca database.
    dcc->send("kded", "kbuildsycoca", "recreate()", QByteArray());
}
