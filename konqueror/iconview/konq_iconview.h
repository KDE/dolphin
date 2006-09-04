/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __konq_iconview_h__
#define __konq_iconview_h__

#include <kparts/browserextension.h>
#include <konq_iconviewwidget.h>
#include <konq_operations.h>
#include <konq_dirpart.h>
#include <kmimetyperesolver.h>
#include <q3ptrdict.h>
#include <q3ptrlist.h>
#include <kfileivi.h>

class KonqPropsView;
class KFileItem;
class KDirLister;
class KAction;
class KToggleAction;
class KActionMenu;
class Q3IconViewItem;
class IconViewBrowserExtension;

/**
 * The Icon View for konqueror.
 * The "Kfm" in the name stands for file management since it shows files :)
 */
class KonqKfmIconView : public KonqDirPart
{
  friend class IconViewBrowserExtension; // to access m_pProps
  Q_OBJECT
  Q_PROPERTY( bool supportsUndo READ supportsUndo )
  Q_PROPERTY( QString viewMode READ viewMode WRITE setViewMode )
public:

  enum SortCriterion { NameCaseSensitive, NameCaseInsensitive, Size, Type, Date };

  KonqKfmIconView( QWidget *parentWidget, QObject *parent, const QString& mode );
  virtual ~KonqKfmIconView();

  virtual const KFileItem * currentItem();
  virtual KFileItemList selectedFileItems() {return m_pIconView->selectedFileItems();};


  KonqIconViewWidget *iconViewWidget() const { return m_pIconView; }

  bool supportsUndo() const { return true; }

  void setViewMode( const QString &mode );
  QString viewMode() const { return m_mode; }

  // "Cut" icons : disable those whose URL is in lst, enable the rest
  virtual void disableIcons( const KUrl::List & lst );

  // See KMimeTypeResolver
  void mimeTypeDeterminationFinished();
  void determineIcon( KFileIVI * item );
  int iconSize() { return m_pIconView->iconSize(); }

public Q_SLOTS:
  void slotPreview( bool toggle );
  void slotShowDirectoryOverlays();
  void slotShowDot();
  void slotSelect();
  void slotUnselect();
  void slotSelectAll();
  void slotUnselectAll();
  void slotInvertSelection();

  void slotSortByNameCaseSensitive( bool toggle );
  void slotSortByNameCaseInsensitive( bool toggle );
  void slotSortBySize( bool toggle );
  void slotSortByType( bool toggle );
  void slotSortByDate( bool toggle );
  void slotSortDescending();
  void slotSortDirsFirst();

protected Q_SLOTS:
  // slots connected to Q3IconView
  void slotReturnPressed( Q3IconViewItem *item );
  void slotMouseButtonPressed(int, Q3IconViewItem*, const QPoint&);
  void slotMouseButtonClicked(int, Q3IconViewItem*, const QPoint&);
  void slotContextMenuRequested(Q3IconViewItem*, const QPoint&);
  void slotOnItem( Q3IconViewItem *item );
  void slotOnViewport();
  void slotSelectionChanged();

  // Slot used for spring loading folders
  void slotDragHeld( Q3IconViewItem *item );
  void slotDragMove( bool accepted );
  void slotDragEntered( bool accepted );
  void slotDragLeft();
  void slotDragFinished();

  // slots connected to the directory lister - or to the kfind interface
  // They are reimplemented from KonqDirPart.
  virtual void slotStarted();
  virtual void slotCanceled();
  void slotCanceled( const KUrl& url );
  virtual void slotCompleted();
  virtual void slotNewItems( const KFileItemList& );
  virtual void slotDeleteItem( KFileItem * );
  virtual void slotRefreshItems( const KFileItemList& );
  virtual void slotClear();
  virtual void slotRedirection( const KUrl & );
  virtual void slotDirectoryOverlayStart();
  virtual void slotDirectoryOverlayFinished();

  /**
   * This is the 'real' finished slot, where we emit the completed() signal
   * after everything was done.
   */
  void slotRenderingFinished();
  // (Re)Draws m_pIconView's contents. Connected to m_pTimeoutRefreshTimer.
  void slotRefreshViewport();

  // Connected to KonqDirPart
  void slotKFindOpened();
  void slotKFindClosed();

protected:
  virtual bool openFile() { return true; }
  virtual bool doOpenURL( const KUrl& );
  virtual bool doCloseURL();

  virtual void newIconSize( int size );

  void setupSorting( SortCriterion criterion );

  /** */
  void setupSortKeys();

  QString makeSizeKey( KFileIVI *item );

  /** The directory lister for this URL */
  KDirLister* m_dirLister;

  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit:1;

  /**
   * Set to true while the dirlister is running, _if_ we asked it
   * explicitly (openUrl). If it is auto-updating, this is not set to true.
   */
  bool m_bLoading:1;

  /**
   * Set to true if we still need to emit completed() at some point
   * (after the loading is finished and after the visible icons have been
   * processed)
   */
  bool m_bNeedEmitCompleted:1;

  /**
   * Set to true if slotCompleted needs to realign the icons
   */
  bool m_bNeedAlign:1;

  bool m_bUpdateContentsPosAfterListing:1;

  bool m_bDirPropertiesChanged:1;

  /**
   * Set in doCloseURL and used in setViewMode to restart a preview
   * job interrupted when switching to IconView or MultiColumnView.
   * Hacks like this must be removed in KDE4!
   */
  bool m_bPreviewRunningBeforeCloseURL:1;

  bool m_bNeedSetCurrentItem:1;

  KFileIVI * m_pEnsureVisible;

  QStringList m_itemsToSelect;

  SortCriterion m_eSortCriterion;

  KToggleAction *m_paDotFiles;
  KToggleAction *m_paDirectoryOverlays;
  KToggleAction *m_paEnablePreviews;
  Q3PtrList<KFileIVI> m_paOutstandingOverlays;
  QTimer *m_paOutstandingOverlaysTimer;
/*  KToggleAction *m_paImagePreview;
  KToggleAction *m_paTextPreview;
  KToggleAction *m_paHTMLPreview;*/
  KActionMenu *m_pamPreview;
  Q3PtrList<KToggleAction> m_paPreviewPlugins;
  KActionMenu *m_pamSort;

  KAction *m_paSelect;
  KAction *m_paUnselect;
  KAction *m_paSelectAll;
  KAction *m_paUnselectAll;
  KAction *m_paInvertSelection;

  KToggleAction *m_paSortDirsFirst;

  KonqIconViewWidget *m_pIconView;

  QTimer *m_pTimeoutRefreshTimer;

  Q3PtrDict<KFileIVI> m_itemDict; // maps KFileItem * -> KFileIVI *

  KMimeTypeResolver<KFileIVI,KonqKfmIconView> * m_mimeTypeResolver;

  QString m_mode;

  private:
  void showDirectoryOverlay(KFileIVI*  item);
};

class IconViewBrowserExtension : public KonqDirPartBrowserExtension
{
  Q_OBJECT
  friend class KonqKfmIconView; // so that it can emit our signals
public:
  IconViewBrowserExtension( KonqKfmIconView *iconView );

  virtual int xOffset();
  virtual int yOffset();

public Q_SLOTS:
  // Those slots are automatically connected by the shell
  void reparseConfiguration();
  void setSaveViewPropertiesLocally( bool value );
  void setNameFilter( const QString &nameFilter );

  void refreshMimeTypes() { m_iconView->iconViewWidget()->refreshMimeTypes(); }

  void rename() { m_iconView->iconViewWidget()->renameSelectedItem(); }
  void cut() { m_iconView->iconViewWidget()->cutSelection(); }
  void copy() { m_iconView->iconViewWidget()->copySelection(); }
  void paste() { m_iconView->iconViewWidget()->pasteSelection(); }
  void pasteTo( const KUrl &u ) { m_iconView->iconViewWidget()->paste( u ); }

  void trash();
  void del() { KonqOperations::del(m_iconView->iconViewWidget(),
                                   KonqOperations::DEL,
                                   m_iconView->iconViewWidget()->selectedUrls()); }
  void properties();
  void editMimeType();

  // void print();

private:
  KonqKfmIconView *m_iconView;
  bool m_bSaveViewPropertiesLocally;
};

class SpringLoadingManager : public QObject
{
    Q_OBJECT
private:
    SpringLoadingManager();
    static SpringLoadingManager *s_self;
public:
    static SpringLoadingManager &self();
    static bool exists();

    void springLoadTrigger(KonqKfmIconView *view, KFileItem *file,
                           Q3IconViewItem *item);

    void dragLeft(KonqKfmIconView *view);
    void dragEntered(KonqKfmIconView *view);
    void dragFinished(KonqKfmIconView *view);

private Q_SLOTS:
    void finished();

private:
    KUrl m_startURL;
    KParts::ReadOnlyPart *m_startPart;

    // Timer allowing to know the user wants to abort the spring loading
    // and go back to his start url (closing the opened window if needed)
    QTimer m_endTimer;
};


#endif
