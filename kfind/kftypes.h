/***********************************************************************
 *
 *  KfFileType.h
 *
 **********************************************************************/

#ifndef KFTYPES_H
#define KFTYPES_H

#include <qstring.h>
#include <qlist.h>
#include <qstrlist.h>
#include <qfile.h>

#include <kapp.h>
#include <kconfig.h>
 
class KfFileType
{
public:
  /*********************************************************
  * Create a KfFileType
  */
  KfFileType() {};

  /*********************************************************
  * Create a KfFileType with pattern.
  */
  KfFileType( const char *_pattern);

  virtual ~KfFileType() {}

  /// Add a pattern which matches this type.
  virtual void addPattern( const char *_p ) 
    { if ( pattern.find( _p ) == -1 ) pattern.append( _p ); }

  /*********************************************************
  * Returns the extensions assoziated with this FileType, for
  * example '.html' or '.cpp'.
  */
  virtual QStrList& getPattern() { return pattern; }

  /// Returns the name of this binding
  /**
     The file type in the file '$KDEDIR/filetypes/Spreadsheet.kdelnk' for
     example is called 'Spreadsheet'.
  */
  const char* getName() { return name.data(); }

  /// Returns this file types comment.
  /**
     The return value may be 0L if there is no comment at all.
     This method does not use _url but some overloading methods do so.
  */
  virtual QString getComment( const char * ) 
    { return QString( comment.data() ); }

  /// Sets the comment
  void setComment( const char *_c) { comment = _c; }
 
  /**
     The return may be 0. In this case the user did not specify
     a special default binding. That does not mean that there is no
     binding at all.
  */
  virtual const char* getDefaultBinding() { return defaultBinding; }

  /// Set the default bindings name
  virtual void setDefaultBinding( const char *_b ) 
    { defaultBinding = _b; defaultBinding.detach(); }
 
  /*********************************************************
  * Tries to find a FileType for the file _url. If no special
  * FileType is found, the default FileType is returned.
  */
  static KfFileType* findType( const char *_url );

  /// Find type by pattern.
  /**
     If for the given pattern no KfFileType has been created, the function
     will retun 0L;
  */
  static KfFileType *findByPattern( const char *_pattern );

  /// Finds a file type by its name
  /**
     The file type in the file '$KDEDIR/filetypes/Spreadsheet.kdelnk' for
     example is called 'Spreadsheet'.
  */
  static KfFileType *findByName( const char *_name );

  /*********************************************************
  * Call this function before you call any other function. This
  * function initializes some global variables.
  */
  static void init();

  /// Scan the $KDEDIR/filetypes directory for the file types
  static void initFileTypes( const char *_path );

  /// Return the path for the icons
  static const char* getIconPath() { return icon_path; }
  static const char* getDefaultPixmap() { return defaultPixmap; }
  static const char* getExecutablePixmap() { return executablePixmap; }
  static const char* getBatchPixmap() { return batchPixmap; }
  static const char* getFolderPixmap() { return folderPixmap; }

protected:
  /*********************************************************
  * List with all bindings for this type.
  */
  //  QList<KFileBind> bindings;

  /*********************************************************
  * The full qualified filename ( not an URL ) of the icons
  * pixmap.
  */
  QString pixmap_file;
    
  /*********************************************************
  * The pattern matching this file. For example: ".html".
  */
  QStrList pattern;

  /// Holds the default binding
  /**
     This string may be 0. In this case the user did not specify
     a special default binding. That does not mean that there is no
     binding at all. Attention: Perhaps a binding with that name does
     not exist for a strange reason.
  */
  QString defaultBinding;

  /// Holds the name of the file type
  /**
     The file type in the file '$KDEDIR/filetypes/Spreadsheet.kdelnk' for
     example is called 'Spreadsheet'.
  */
  QString name;

  /// Holds a comment to this file type.
  QString comment;
  /// The path to the icons
  static char icon_path[ 1024 ];
  /// Default pixmap for executables
  static char executablePixmap[ 1024 ];
  /// Default pixmap for batch files
  static char batchPixmap[ 1024 ];
  /// General default pixmap
  static char defaultPixmap[ 1024 ];
  /// Default pixmap for folders
  static char folderPixmap[ 1024 ];
};
      
#endif
