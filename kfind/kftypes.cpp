/***********************************************************************
 *
 *  KfFileType.cpp
 *
 **********************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>

#include <qmsgbox.h>
#include <qtstream.h>

#include "kftypes.h"

QList<KfFileType> *types; 

void KfFileType::initFileTypes( const char* _path )
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir( _path );
    if ( dp == NULL )
        return;

    // Loop thru all directory entries
    while ( ( ep = readdir( dp ) ) != NULL )
    {
        if ( strcmp( ep->d_name, "." ) != 0 && strcmp( ep->d_name, ".." ) != 0 )
        {
            QString tmp = ep->d_name;

            QString file = _path;
            file += "/";
            file += ep->d_name;
            struct stat buff;
            stat( file.data(), &buff );
            if ( S_ISDIR( buff.st_mode ) )
                initFileTypes( file.data() );
            else if ( tmp.right( 7 ) == ".kdelnk" )
            {
                QFile f( file.data() );
                if ( !f.open( IO_ReadOnly ) )
                    return;

                QTextStream pstream( &f );
                KConfig config( &pstream );
                config.setGroup( "KDE Desktop Entry" );

                // Read a new extension group
                QString ext = ep->d_name;
                if ( ext.right(7) == ".kdelnk" )
                    ext = ext.left( ext.length() - 7 );

                // Get a ';' separated list of all pattern
                QString pats = config.readEntry( "Patterns" );

                QString icon = config.readEntry( "Icon" );
                QString defapp = config.readEntry( "DefaultApp" );
                QString comment = config.readEntry( "Comment" );

               // Take this type only of it has pattern
               if ( (!pats.isNull() && pats!="") &&  pats!=";" )
                 {
                   // Is this file type already registered ?
                   KfFileType *t = KfFileType::findByName( ext.data() );
                   // If not then create a new type, 
                   //but only if we have an icon
                   if ( t == 0L && !icon.isNull() )
                     types->append( t = new KfFileType(ext.data()));
                   
                   // Set the default binding
                   if ( !defapp.isNull() && t != 0L )
                     t->setDefaultBinding( defapp.data() );
                   if ( t != 0L )
                     t->setComment( comment.data() );

                   int pos2 = 0;
                   int old_pos2 = 0;
                   while ( ( pos2 = pats.find( ";", pos2 ) ) != - 1 )
                     {
                       // Read a pattern from the list
                       QString name = pats.mid( old_pos2, pos2 - old_pos2 );
                       if ( t != 0L )
                         t->addPattern( name.data() );
                       pos2++;
                       old_pos2 = pos2;
                     }
                 };
                f.close();
            }
        }
    }
}
     
void KfFileType::init()
{
  types = new QList<KfFileType>;

  // Read the filetypes
  QString path = KApplication::kdedir();
  path += "/share/mimelnk";
  initFileTypes( path.data() );
};

KfFileType* KfFileType::findByName( const char *_name )
{
  KfFileType *typ;

  for ( typ = types->first(); typ != 0L; typ = types->next() )
    {
      if ( strcmp( typ->getName(), _name ) == 0 )
	return typ;
    };

  return 0L;
};

KfFileType* KfFileType::findByPattern( const char *_pattern )
{
    KfFileType *typ;

    for ( typ = types->first(); typ != 0L; typ = types->next() )
    {
        QStrList & pattern = typ->getPattern();
        char *s;
        for ( s = pattern.first(); s != 0L; s = pattern.next() )
            if ( strcmp( s, _pattern ) == 0 )
                return typ;
    }

    return 0L;
};
  
KfFileType::KfFileType( const char *_name)
{
    name = _name;

    //    HTMLImage::cacheImage( pixmap_file.data() );
};






