/***********************************************************************
 *
 *  KfSaveOptions.h
 *
 **********************************************************************/

class KfSaveOptions
{
public:
  // Initialize KfSaveOptions object
  /* Scans konfiguration file for saving parameters
   */
  KfSaveOptions();

  // returns filename
  const char* getSaveFile() { return saveFile; };

  // returns file format (HTML or Plain Text
  const char* getSaveFormat() { return saveFormat; };

  //aks whether standart output file shoud be used
  bool getSaveStandard() { return saveStandard; };;

  // sets filename
  void setSaveFile(const char*);

  //sets format
  void setSaveFormat(const char*);

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
