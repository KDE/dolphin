/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef kfindpart__h
#define kfindpart__h

#include <kparts/browserextension.h>
#include <kparts/part.h>
#include <kfileitem.h>
#include <kdebug.h>
#include <qlist.h>
#include <konq_dirpart.h>

class KQuery;
//added
class KonqPropsView;
class KonqDirLister;
class KonqFileItem;
class KAction;
class KToggleAction;
class KActionMenu;
class QIconViewItem;
class IconViewBrowserExtension;
//end added

class KFindPart : public KonqDirPart//KParts::ReadOnlyPart
{
  friend class KFindPartBrowserExtension;
    Q_OBJECT
    Q_PROPERTY( bool showsResult READ showsResult )
public:
    KFindPart( QWidget * parentWidget, QObject *parent, const char *name, const QString& mode );
    virtual ~KFindPart();

    virtual bool openURL( const KURL &url );
    virtual bool openFile() { return false; }

    bool showsResult() const { return m_bShowsResult; }
    /* Save everything in the dialog box, useful for the "back" function of konqueror */
    void saveKFindState( QDataStream *stream );
    void restoreKFindState( QDataStream *stream );

  // "Cut" icons : disable those whose URL is in lst, enable the rest //added for konqdirpart
  virtual void disableIcons( const KURL::List & ){};
  virtual const KFileItem * currentItem(){return 0;};

signals:
    // Konqueror connects directly to those signals
    void started(); // started a search
    void clear(); // delete all items
    void newItems(const KFileItemList&); // found this/these item(s)
    void finished(); // finished searching
    void canceled(); // the user canceled the search
    void findClosed(); // close us

protected slots:
    void slotStarted();
    void slotDestroyMe();
    void addFile(const KFileItem *item);
    void slotResult(int errorCode);

  // slots connected to the directory lister  //added for konqdirpart
//  virtual void slotStarted();
  virtual void slotCanceled(){};
  virtual void slotCompleted(){};
  virtual void slotNewItems( const KFileItemList& ){};
  virtual void slotDeleteItem( KFileItem * ){};
  virtual void slotRefreshItems( const KFileItemList& ){};
  virtual void slotClear(){};
  virtual void slotRedirection( const KURL & ){};

private:
    Kfind * m_kfindWidget;
    KQuery *query;
    bool m_bShowsResult; // whether the dirpart shows the results of a search or not
    /**
     * The internal storage of file items
     */
    QList<KFileItem> m_lstFileItems;
};

/* This class will be used to save the kfind dialog state and
  the search result. These will be restored when the user press the
  "back" button
*/
class KFindPartBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  friend class KFindPart;
public:
  KFindPartBrowserExtension( KFindPart *findPart );

  virtual void saveState( QDataStream &stream )
    {
      KParts::BrowserExtension::saveState( stream );
      m_findPart->saveKFindState( &stream );
    }

  virtual void restoreState( QDataStream &stream )
    {
      KParts::BrowserExtension::restoreState( stream );
      m_findPart->restoreKFindState( &stream );
    }

private:
  KFindPart *m_findPart;
  bool m_bSaveViewPropertiesLocally;
};

#endif
