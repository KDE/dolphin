/***********************************************************************
 *
 *  KfSaveOptions.h
 *
 **********************************************************************/

#ifndef _KFSAVE_H_
#define _KFSAVE_H_

class KfSaveOptions
{
public:
  // Initialize KfSaveOptions object
  /* Scans konfiguration file for saving parameters
   */
  KfSaveOptions();

  // returns filename
  QString getSaveFile() { return saveFile; };

  // returns file format (HTML or Plain Text
  QString getSaveFormat() { return saveFormat; };

  //aks whether standart output file shoud be used
  bool getSaveStandard() { return saveStandard; };

  // sets filename
  void setSaveFile(const QString&);

  //sets format
  void setSaveFormat(const QString&);

  //sets whether standart output file shoud be used
  void setSaveStandard(bool);

private:

  // Name of file in which the results will be stored
  QString saveFile;
  
  // Format of file. Can be HTML or Plai Text
  QString saveFormat;

  // If TRUE the results will be saved to file ~/.kfind-results.html
  bool saveStandard;
};

#endif
