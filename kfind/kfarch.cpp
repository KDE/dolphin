/***********************************************************************
 *
 *  KfArchiver.cpp
 *
 **********************************************************************/

#include <kapp.h>
#include <string.h>

#include "kfarch.h"

QList<KfArchiver> *archivers;        

// Create KfArchivers object
KfArchiver::KfArchiver(const char *_archiver)
{
  arName = _archiver;
};

/// Initialize archivers global varibles
void KfArchiver::init()
{
  archivers = new QList<KfArchiver>;

  // Read the archivers
  initArchivers();
};

/// This function scans file ~/.kfindrc for archivers
void KfArchiver::initArchivers()
{
  QString name;

  KConfig *config = KApplication::getKApplication()->getConfig();
  config->setGroup( "Archiver Types" );

  QString arch = config->readEntry( "Archivers" );

  //Create Tar Archive Entry when no entry found in rc file
  if ( arch.isNull() | (arch=="") )
    {
      arch="tar;";
      config->setGroup( "Archiver Types" );
      config->writeEntry( "Archivers", arch.data() );

      config->setGroup( "tar" );
      config->writeEntry( "Comment", "Tar" );
      config->writeEntry( "ExecOnCreate", "tar cf %a -C %d %n" );
      config->writeEntry( "ExecOnUpdate", "tar uf %a -C %d %n" );
      config->writeEntry( "Pattern", "*.tar;" );
    };

  int pos = 0;
  int old_pos = 0;
  QStrList names; // Temporally stores names of archives
  
  while ( ( pos = arch.find( ";", pos ) ) != - 1 )
    {
      // Read a archiver names from the list
      name = arch.mid( old_pos, pos - old_pos );
      if (names.find(name.data()) == -1 )
	names.append( name.data() );
      pos++;
      old_pos = pos;
    };

  for (name =  names.first(); name!=0L; name = names.next())
    {
      config->setGroup( name.data() );
      
      QString comment   = config->readEntry( "Comment" );
      QString oncreate  = config->readEntry( "ExecOnCreate" );
      QString onupdate  = config->readEntry( "ExecOnUpdate" );
      QString patterns  = config->readEntry( "Pattern" );
      
      if ( !( (oncreate.isNull() ) | (strcmp(oncreate.data(),"")==0) 
	    | (onupdate.isNull() ) | (strcmp(onupdate.data(),"")==0) ) )
	{
	  KfArchiver *ar   = new KfArchiver( name.data() );
	  ar -> setComment(comment);
	  ar -> setOnCreate(oncreate);
	  ar -> setOnUpdate(onupdate);
	  
	  pos=0;
	  old_pos=0;
	  while ( ( pos = patterns.find( ";", pos ) ) != - 1 )
	    {
	      // Read a pattern from the list
	      QString pattern = patterns.mid( old_pos, pos - old_pos );
	      ar->addPattern( pattern.data() );
	      pos++;
	      old_pos = pos;
	    };
	  archivers->append(ar);
	};	
    };
  
};

KfArchiver* KfArchiver::findByPattern( const char *_pattern )
{
  KfArchiver *arch;

  for ( arch = archivers->first(); arch != 0L; arch = archivers->next() )
    {
      QStrList & pattern = arch->getArPattern();
      char *s;
      for ( s = pattern.first(); s != 0L; s = pattern.next() )
	if ( strcmp( s, _pattern ) == 0 )
	  return arch;
    };
  
  return 0L;
};


