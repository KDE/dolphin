/***************************************************************************
  Copyright:
  (C) 2002 by George Russell <george.russell@clara.net>
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef KHTMLKTTSD_H
#define KHTMLKTTSD_H

#include <kparts/plugin.h>
#include <klibloader.h>

class KURL;
class KInstance;

/**
 * A plugin is the way to add actions to an existing @ref KParts application,
 * or to a @ref Part.
 *
 * The XML of those plugins looks exactly like of the shell or parts,
 * with one small difference: The document tag should have an additional
 * attribute, named "library", and contain the name of the library implementing
 * the plugin.
 *
 * If you want this plugin to be used by a part, you need to
 * install the rc file under the directory
 * "data" (KDEDIR/share/apps usually)+"/instancename/kpartplugins/"
 * where instancename is the name of the part's instance.
 **/
class KHTMLPluginKTTSD : public KParts::Plugin
{
    Q_OBJECT
public:

    /**
     * Construct a new KParts plugin.
     */
    KHTMLPluginKTTSD( QObject* parent = 0, const char* name = 0 );

    /**
     * Destructor.
     */
    virtual ~KHTMLPluginKTTSD();
public slots:
    void slotReadOut();
};
 

/**
 * If you develop a library that is to be loaded dynamically at runtime, then
 * you should provide a function that returns a pointer to your factory like this:
 * <pre>
 * extern "C"
 * {
 *   void* init_libkspread()
 *   {
 *     return new KSpreadFactory;
 *   }
 * };
 * </pre>
 * You should especially see that the function must follow the naming pattern
 * "init_libname".
 *
 * In the constructor of your factory you should create an instance of @ref KInstance
 * like this:
 * <pre>
 *     s_global = new KInstance( "kspread" );
 * </pre>
 * This @ref KInstance is compareable to @ref KGlobal used by normal applications.
 * It allows you to find ressource files (images, XML, sound etc.) belonging
 * to the library.
 *
 * If you want to load a library, use @ref KLibLoader. You can query @ref KLibLoader
 * directly for a pointer to the libraries factory by using the @ref KLibLoader::factory()
 * function.
 *
 * The KLibFactory is used to create the components, the library has to offer.
 * The factory of KSpread for example will create instances of KSpreadDoc,
 * while the KHTMLueror factory will create KHTMLView widgets.
 * All objects created by the factory must be derived from @ref QObject, since @ref QObject
 * offers type safe casting.
 *
 * KLibFactory is an abstract class. Reimplement the @ref
 * createObject() method to give it functionality.
 *
 * @author Torben Weis <weis@kde.org>
 */
class KPluginFactory : public KLibFactory
{
  Q_OBJECT
public:

    /**
     * Create a new factory.
     */
  KPluginFactory( QObject *parent = 0, const char *name = 0 );

    /**
     * Destroy factory.
     */
  ~KPluginFactory() ;


    /**
     * Creates a new object. The returned object has to be derived from
     * the requested classname.
     *
     * It is valid behavior to create different kinds of objects
     * depending on the requested @p classname. For example a koffice
     * library may usually return a pointer to @ref KoDocument.  But
     * if asked for a "QWidget", it could create a wrapper widget,
     * that encapsulates the Koffice specific features.
     *
     * Never reimplement this function. Instead, reimplement @ref
     * createObject().
     *
     * create() automatically emits a signal @ref objectCreated to tell
     * the library about its newly created object.  This is very
     * important for reference counting, and allows unloading the
     * library automatically once all its objects have been destroyed.
     *
     * This function is virtual for compatibility reasons only.
     */
  virtual QObject* createObject( QObject* parent = 0, const char* pname = 0,
                           const char* name = "QObject",
                           const QStringList &args = QStringList() );

private:
  static KInstance* s_instance;
};
 
#endif
