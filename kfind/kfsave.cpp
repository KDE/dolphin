/***********************************************************************
 *
 *  KfSaveOptions.cpp
 *
 **********************************************************************/

#include <qstring.h>
#include <kapp.h>

#include "kfsave.h"

// Initialize KfSaveOptions object
/* Scans konfiguration file for saving parameters
 */
KfSaveOptions::KfSaveOptions()
{
  // Read the configuration
  KConfig *config = KApplication::getKApplication()->getConfig();
  config->setGroup( "Saving" );
  
  saveFormat = (config->readEntry( "Format" )).upper();
  if ( saveFormat.isNull() | (saveFormat=="") )
    saveFormat = "HTML";

  saveFile   = config->readEntry( "Filename" );
  if ( saveFile.isNull() | (saveFile=="") )
    saveStandard = TRUE;
  else
    saveStandard = FALSE;

  if ( saveFormat!="HTML" )
    saveFormat = "Plain Text";

};

// sets filename
void KfSaveOptions::setSaveFile(const char *_file)
{
  saveFile = _file;
};

//sets format
void KfSaveOptions::setSaveFormat(const char *_format)
{
  saveFormat = _format;
};

//sets whether standart output file shoud be used
void KfSaveOptions::setSaveStandard(bool _std)
{
  saveStandard = _std;
};
