/***********************************************************************
 *
 *  KfSaveOptions.cpp
 *
 **********************************************************************/

#include <qstring.h>
#include <kapp.h>

#include "kfsave.h"
#include <kconfig.h>

// Initialize KfSaveOptions object
/* Scans konfiguration file for saving parameters
 */
KfSaveOptions::KfSaveOptions()
{
  // Read the configuration
  KConfig *config = KApplication::kApplication()->config();
  config->setGroup( "Saving" );
  
  saveFormat = QString(config->readEntry( "Format" )).upper();
  if ( saveFormat.isEmpty() )
    saveFormat = "HTML";

  saveFile   = config->readEntry( "Filename" );
  if ( saveFile.isEmpty() )
    saveStandard = TRUE;
  else
    saveStandard = FALSE;

  if ( saveFormat!="HTML" )
    saveFormat = "Plain Text";

};

// sets filename
void KfSaveOptions::setSaveFile(const QString& _file)
{
  saveFile = _file;
};

//sets format
void KfSaveOptions::setSaveFormat(const QString& _format)
{
  saveFormat = _format;
};

//sets whether standart output file shoud be used
void KfSaveOptions::setSaveStandard(bool _std)
{
  saveStandard = _std;
};
