
#ifndef __KonquerorIface_h__
#define __KonquerorIface_h__

#include <dcopobject.h>
#include <qvaluelist.h>
#include <dcopref.h>

/**
 * DCOP interface for konqueror
 */
class KonquerorIface : virtual public DCOPObject
{
  K_DCOP
public:

  KonquerorIface();
  ~KonquerorIface();

k_dcop:
  /**
   * Opens a new window for the given @p url
   */
  void openBrowserWindow( const QString &url );

  /**
   * As the name says, this creates a window from a profile.
   * Used for instance by khelpcenter.
   */
  ASYNC createBrowserWindowFromProfile( const QString &filename );

  /**
   * This is for the "cut" feature - we tell all konqueror/kdesktop instances that
   * after paste we should remove the original
   */
  ASYNC setMoveSelection( int move );

  /**
   * Called by kcontrol when the global configuration changes
   */
  ASYNC reparseConfiguration();

  /**
   * @return a list of references to all the windows
   */
  QValueList<DCOPRef> getWindows();

};

#endif

