/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#ifndef __kdirlister_h__
#define __kdirlister_h__

#include "kfileitem.h"

#include <qtimer.h>
#include <qstringlist.h>
#include <qlist.h>

#include <kurl.h>
#include <kio_interface.h>

/** The dir lister deals with the kiojob used to list and update a directory,
 * handles the timer, and has signals for the user of this class (e.g. konqueror
 * view or kdesktop) to create/destroy its items when asked.
 *
 * This class is independent from the graphical representation of the dir
 * (icon container, tree view, ...) and it stores the items (as KFileItems).
 *
 * Typical usage :
 * Create an instance,
 * Connect to at least update, clear, newItem, and deleteItem
 * Call openURL - the signals will be called
 * Reuse the instance when opening a new url (openURL)
 * Destroy the instance when not needed anymore (usually destructor)
 *
 * Advanced usage : call openURL with _bKeep = true to list directories
 * without forgetting the ones previously read (e.g. for a tree view)
 */
class KDirLister : public QObject
{
  Q_OBJECT
public:
  /**
   * Create a directory lister
   */
  KDirLister();
  /**
   * Destroy the directory lister
   */
  ~KDirLister();
  
  /**
   * Run the directory lister on the given url
   * @param _url the directory URL
   * @param _showDotFiles whether to return the "hidden" files
   * @param _keep if true the previous directories aren't forgotten
   * (they are still watched by kdirwatch and their items are kept in
   * m_lstFileItems)
   */
  virtual void openURL( const KURL& _url, bool _showDotFiles, bool _keep = false );

  /**
   * Stop listing the current directory
   */
  virtual void stop() { slotCloseURL( 0 ); }

  /**
   * @return the url used by this instance to list the files
   * It might be different from the one we gave, if there was a redirection
   * (TODO : redirections not implemented)
   */
  virtual QString url() const { return m_sURL; }

  /**
   * @return the url used by this instance to list the files
   * It might be different from the one we gave, if there was a redirection.
   */
  virtual const KURL & kurl() const { return m_url; }

  /** 
   * Update the currently displayed directory
   * The current implementation calls it automatically for
   * local files, using KDirWatch, but it might be useful to force an
   * update manually.
   */
  virtual void updateDirectory( const QString& _dir );

  /**
   * Changes the "is viewing dot files" setting.
   * Calls updateDirectory() if setting changed
   */
  virtual void setShowingDotFiles( bool _showDotFiles );
  
  /**
   * Find an item
   * @param _url the item URL
   * @return the pointer to the KFileItem
   **/
  KFileItem* find( const QString& _url );

  /**
   * @return the list of file items currently displayed
   */
  QList<KFileItem> & items() { return m_lstFileItems; }
  
signals:
  /**
   * Tell the view that we started to list _url.
   * The view knows that openURL should start it, so it might seem useless, 
   * but the view also needs to know when an automatic update happens.
   */
  void started( const QString& _url );

  /** Tell the view that listing is finished */
  void completed();
  /** Tell the view that user canceled the listing */
  void canceled();

  /** Clear all items */
  void clear();
  /** Signal a new item */
  void newItem( KFileItem * _fileItem );
  /** Signal a item to remove */
  void deleteItem( KFileItem * _fileItem );

protected slots:
  // internal slots used by the directory lister (connected to the job)
  virtual void slotCloseURL( int _id );
  virtual void slotListEntry( int _id, const UDSEntry& _entry );
  virtual void slotError( int _id, int _errid, const char *_errortext );
  virtual void slotBufferTimeout();
  virtual void slotUpdateError( int _id, int _errid, const char *_errortext );
  virtual void slotUpdateFinished( int _id );
  virtual void slotUpdateListEntry( int _id, const UDSEntry& _entry );

  // internal slots connected to KDirWatch
  virtual void slotDirectoryDirty( const QString& _dir );
  virtual void slotFileDirty( const QString& _file );

protected:  
  /** Unregister dirs from kdirwatch and clear m_lstDirs */
  virtual void forgetDirs();

  /** The url that we used to list (can be different in case of redirect) */
  KURL m_url;
  QString m_sURL;

  /** Did we found the first file in the dir ?
      Set to false by openURL and to true by slotBufferTimeout */
  bool m_bFoundOne;
  bool m_bIsLocalURL;
  
  int m_jobId;

  /** The internal storage of file items */
  QList<KFileItem> m_lstFileItems;

  /**
   * List of dirs handled by this instance. Same as m_sURL if only one dir
   * But for a tree view, it contains all the dirs shown
   * (Used to unregister from kdirwatch)
   */
  QStringList m_lstDirs;

  bool m_isShowingDotFiles;
  bool m_bComplete;
  /** Used internally between starting-of-listing and first timeout */
  QString m_sWorkingURL;

  QValueList<UDSEntry> m_buffer;
  QTimer m_bufferTimer;
  
  QStringList m_lstPendingUpdates;
};

#endif
