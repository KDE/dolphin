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

#include <qmessagebox.h>
#include <qtextstream.h>

#include "kftypes.h"

#include <kstddirs.h>
#include <kglobal.h>

QList<KfFileType> *types; 

void KfFileType::initFileTypes( const QString& _path )
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir( _path.ascii() );
    if ( dp == NULL )
        return;

    // Loop thru all directory entries
    while ( ( ep = readdir( dp ) ) != NULL )
    {
        if ( strcmp( ep->d_name, "." ) != 0 && strcmp( ep->d_name, ".." ) != 0 )
        {
            QString tmp = ep->d_name;

            QString file = _path;
            file += '/';
            file += ep->d_name;
            struct stat buff;
            stat( file.ascii(), &buff );
            if ( S_ISDIR( buff.st_mode ) )
                initFileTypes( file );
            else if ( tmp.right( 8 ) == ".desktop" )
            {
                QFile f( file );
                if ( !f.open( IO_ReadOnly ) )
                    return;

				f.close(); // kalle
				// kalle                QTextStream pstream( &f );
                KConfig config( file ); //
                config.setDesktopGroup();

                // Read a new extension group
                QString ext = ep->d_name;
                if ( ext.right( 8 ) == ".desktop" )
                    ext = ext.left( ext.length() - 8 );

                // Get a ';' separated list of all pattern
                QString pats = config.readEntry( "Patterns" );

                QString icon = config.readEntry( "Icon" );
                QString defapp = config.readEntry( "DefaultApp" );
                QString comment = config.readEntry( "Comment" );

		// Take this type only of it has pattern
		if ( (!pats.isNull() && pats!="") &&  pats!=";" )
		  {
		    // Is this file type already registered ?
		    KfFileType *t = KfFileType::findByName( ext );
		    // If not then create a new type, 
		    //but only if we have an icon
		    if ( t == 0L && !icon.isNull() )
		      types->append( t = new KfFileType(ext));
		    
		    // Set the default binding
		    if ( !defapp.isNull() && t != 0L )
		      t->setDefaultBinding( defapp );
		    if ( t != 0L )
		      t->setComment( comment );

		    int pos2 = 0;
		    int old_pos2 = 0;
		    while ( ( pos2 = pats.find( ";", pos2 ) ) != - 1 )
		      {
			// Read a pattern from the list
			QString name = pats.mid( old_pos2, pos2 - old_pos2 );
			if ( t != 0L )
			  t->addPattern( name );
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

  QStringList list = KGlobal::dirs()->getResourceDirs("mime");
  for (QStringList::ConstIterator it = list.begin(); it != list.end(); it++)
    initFileTypes( *it );
};

KfFileType* KfFileType::findByName( const QString& _name )
{
  KfFileType *typ;

  for ( typ = types->first(); typ != 0L; typ = types->next() )
    {
      if (typ->getName() == _name)
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
};






