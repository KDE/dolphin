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

#include <qdir.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>

#include <kapp.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klibloader.h>
#include <kconfig.h>
#include <kdesktopfile.h>

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


void generateMimeType( QString mime, QString extensions, QString description )
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
        ts << "Comment=Supported by Netscape plugin" << endl;
        ts << "X-KDE-AutoEmbed=true" << endl;
        ts << "X-KDE-nsplugin=true" << endl;
        ts << "InitialPreference=0" << endl;

        if (!extensions.isEmpty()) {
            ts << "Patterns=";
            QStringList exts = QStringList::split(",", extensions);
            for (QStringList::Iterator it=exts.begin(); it != exts.end(); ++it)
                ts << "*." << (*it).stripWhiteSpace() << ";";
            ts << endl;
        }

        if (!description.isEmpty())
            ts << "Name=" << description << endl;
        else
            ts << "Name=Netscape plugin mimeinfo" << endl;

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
             files[i]=="cult3dplugin.so" )      // Cult 3d plugin
            continue;

        // get absolute file path
        QString absFile = files.absFilePath( files[i] );
        kdDebug(1433) << "Checking library " << absFile << endl;

        // open the library and ask for the mimetype
        kdDebug(1433) << " - opening" << absFile << endl;
        KLibrary *_handle = KLibLoader::self()->library( absFile.latin1() );

        if (!_handle) {
            kdDebug(1433) << " - open failed, skipping " << endl;
            continue;
        }

        // ask for name and description
        QString name = "Unnamed plugin";
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

        // get mime description function pointer
        char* (*func_GetMIMEDescription)() =
            (char *(*)())_handle->symbol("NP_GetMIMEDescription");
        if ( !func_GetMIMEDescription ) {
            kdDebug(1433) << " - no GetMIMEDescription, skipping" << endl;
            KLibLoader::self()->unloadLibrary( absFile.latin1() );
            continue;
        }

        // ask for mime information
        QString mimeInfo = func_GetMIMEDescription();
        if ( mimeInfo.isEmpty() ) {
            kdDebug(1433) << " - no mime info returned, skipping" << endl;
            KLibLoader::self()->unloadLibrary( absFile.latin1() );
            continue;
        }

        // remove version info, as it is not used at the moment
        QRegExp versionRegExp(";version=[^:]*:");
        mimeInfo.replace( versionRegExp, ":");

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
                mimeInfoList.append( *type );
        }

        // register plugin for javascript
        registerPlugin( name, description, files[i], mimeInfo );

        // unload plugin lib
        kdDebug(1433) << " - unloading  plugin" << endl;
        KLibLoader::self()->unloadLibrary( absFile.latin1() );
    }

    // iterate over all sub directories
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
        ts << "Name=Netscape plugin viewer" << endl;
        ts << "Type=Service" << endl;
        ts << "Icon=netscape" << endl;
        ts << "Comment=Netscape plugin viewer" << endl;
        ts << "X-KDE-Library=libnsplugin" << endl;
        ts << "InitialPreference=0" << endl;
        ts << "ServiceTypes=KParts/ReadOnlyPart,Browser/View" << endl;

        if (mimeTypes.count() > 0) {
            ts << "MimeType=";
            for ( QStringList::Iterator it=mimeTypes.begin();
                  it != mimeTypes.end(); ++it)
                ts << *it << ";";
            ts << endl;
        }

        f.close();
    } else
        kdDebug(1433) << "Failed to open file " << fname << endl;
}


int main( int argc, char *argv[] )
{
    KApplication app(argc, argv, "pluginscan");

    // set up the paths used to look for plugins
    QStringList searchPaths = getSearchPaths();
    QStringList mimeInfoList;

    infoConfig = new KConfig( KGlobal::dirs()->saveLocation("data", "nsplugins")+"/pluginsinfo" );

    // open the cache file for the mime information
    QString cacheName = KGlobal::dirs()->saveLocation("data", "nsplugins")+"/cache";
    kdDebug(1433) << "Creating MIME cache file " << cacheName << endl;
    QFile cachef(cacheName);
    if (!cachef.open(IO_WriteOnly))
        return -1;
    QTextStream cache(&cachef);

    // read in the plugins mime information
    kdDebug(1433) << "Scanning directories" << endl;
    for ( QStringList::Iterator it = searchPaths.begin();
          it != searchPaths.end(); ++it)
        scanDirectory( *it, mimeInfoList, cache );

    // delete old mime types
    kdDebug(1433) << "Removing old mimetypes" << endl;
    deletePluginMimeTypes();

    // write mimetype files
    kdDebug(1433) << "Creating MIME type descriptions" << endl;
    QStringList mimeTypes;
    for ( QStringList::Iterator it=mimeInfoList.begin();
          it!=mimeInfoList.end(); ++it) {

      kdDebug(1433) << "Handling MIME type " << *it << endl;

      QStringList info = QStringList::split(":", *it, true);
      if ( info.count()==3 ) {
          QString type = info[0].lower();
          QString extension = info[1];
          QString desc = info[2];

          // append to global mime type list
          if ( !mimeTypes.contains(type) ) {
              kdDebug(1433) << " - mimeType=" << type << endl;
              mimeTypes.append( type );
          }

          // check mimelnk file
          QString fname = KGlobal::dirs()->findResource("mime", type+".desktop");
          if ( fname.isEmpty() || isPluginMimeType(fname) ) {
              kdDebug(1433) << " - creating MIME type description" << endl;
              generateMimeType( type, extension, desc );
          } else
              kdDebug(1433) << " - already existant" << endl;
        }
    }

    // close files
    kdDebug(1433) << "Closing cache file" << endl;
    cachef.close();

    infoConfig->sync();
    delete infoConfig;

    // write plugin lib service file
    writeServicesFile( mimeTypes );
}
