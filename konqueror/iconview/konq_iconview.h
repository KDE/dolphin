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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __konq_iconview_h__
#define __konq_iconview_h__

#include <kparts/browserextension.h>
#include <konq_iconviewwidget.h>
#include <konq_operations.h>
#include <konq_dirpart.h>
#include <konq_mimetyperesolver.h>
#include <qptrdict.h>
#include <kfileivi.h>

class KonqPropsView;
class KonqDirLister;
class KonqFileItem;
class KAction;
class KToggleAction;
class KActionMenu;
class QIconViewItem;
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

  enum SortCriterion { NameCaseSensitive, NameCaseInsensitive, Size, Type };

  KonqKfmIconView( QWidget *parentWidget, QObject *parent, const char *name, const QString& mode );
  virtual ~KonqKfmIconView();

  virtual bool openURL( const KURL &_url );
  virtual bool closeURL();

  virtual bool openFile() { return true; }

  virtual void restoreState( QDataStream &stream );

  virtual const KFileItem * currentItem();
  virtual KFileItemList selectedFileItems() {return m_pIconView->selectedFileItems();};


  KonqIconViewWidget *iconViewWidget() const { return m_pIconView; }

  bool supportsUndo() const { return true; }

  void setViewMode( const QString &mode );
  QString viewMode() const { return m_mode; }

  // "Cut" icons : disable those whose URL is in lst, enable the rest
  virtual void disableIcons( const KURL::List & lst );

  // See KonqMimeTypeResolver
  void mimeTypeDeterminationFinished();
  void determineIcon( KFileIVI * item );
  int iconSize() { return m_pIconView->iconSize(); }

public slots:
  void slotPreview( bool toggle );
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
  void slotSortDescending();
  void slotSortDirsFirst();

protected slots:
  // slots connected to QIconView
  void slotReturnPressed( QIconViewItem *item );
  void slotMouseButtonPressed(int, QIconViewItem*, const QPoint&);
  void slotMouseButtonClicked(int, QIconViewItem*, const QPoint&);
  void slotOnItem( QIconViewItem *item );
  void slotOnViewport();
  void slotSelectionChanged();

  // slots connected to the directory lister
  virtual void slotStarted();
  virtual void slotCanceled();
  virtual void slotCompleted();
  virtual void slotNewItems( const KFileItemList& );
  virtual void slotDeleteItem( KFileItem * );
  virtual void slotRefreshItems( const KFileItemList& );
  virtual void slotClear();
  virtual void slotRedirection( const KURL & );
  void slotCloseView();

  void slotViewportAdjusted() { m_mimeTypeResolver->slotViewportAdjusted(); }
  void slotProcessMimeIcons() { m_mimeTypeResolver->slotProcessMimeIcons(); }

  /**
   * This is the 'real' finished slot, where we emit the completed() signal
   * after everything was done.
   */
  void slotRenderingFinished();

  // Connected to KonqDirPart
  void slotKFindOpened();
  void slotKFindClosed();

protected:

  virtual void newIconSize( int size );

  void setupSorting( SortCriterion criterion );

  /** */
  void setupSortKeys();

  QString makeSizeKey( KFileIVI *item );

  /** The directory lister for this URL */
  KonqDirLister* m_dirLister;

  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit:1;

  /**
   * Set to true while the dirlister is running, _if_ we asked it
   * explicitely (openURL). If it is auto-updating, this is not set to true.
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

  SortCriterion m_eSortCriterion;

  KToggleAction *m_paDotFiles;
/*  KToggleAction *m_paImagePreview;
  KToggleAction *m_paTextPreview;
  KToggleAction *m_paHTMLPreview;*/
  KActionMenu *m_pamPreview;
  QList<KToggleAction> m_paPreviewPlugins;
  KActionMenu *m_pamSort;

  KAction *m_paSelect;
  KAction *m_paUnselect;
  KAction *m_paSelectAll;
  KAction *m_paUnselectAll;
  KAction *m_paInvertSelection;

  KToggleAction *m_paSortDirsFirst;

  KonqIconViewWidget *m_pIconView;

  QPtrDict<KFileIVI> m_itemDict; // maps KFileItem * -> KFileIVI *

  int m_xOffset, m_yOffset;

  KonqMimeTypeResolver<KFileIVI,KonqKfmIconView> * m_mimeTypeResolver;

  QString m_mode;
};

class IconViewBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT
  friend class KonqKfmIconView; // so that it can emit our signals
public:
  IconViewBrowserExtension( KonqKfmIconView *iconView );

  virtual int xOffset();
  virtual int yOffset();

  virtual void saveState( QDataStream &stream )
    {
      m_iconView->saveNameFilter( stream );
      KParts::BrowserExtension::saveState( stream );
      m_iconView->saveState( stream );
    }

  virtual void restoreState( QDataStream &stream )
    {
      // Note: since we need to restore the name filter,
      // BEFORE opening the URL.
      m_iconView->restoreNameFilter( stream );
      KParts::BrowserExtension::restoreState( stream );
      m_iconView->restoreState( stream );
    }

public slots:
  // Those slots are automatically connected by the shell
  void reparseConfiguration();
  void setSaveViewPropertiesLocally( bool value );
  void setNameFilter( QString nameFilter );

  void refreshMimeTypes() { m_iconView->iconViewWidget()->refreshMimeTypes(); }

  void rename() { m_iconView->iconViewWidget()->renameSelectedItem(); }
  void cut() { m_iconView->iconViewWidget()->cutSelection(); }
  void copy() { m_iconView->iconViewWidget()->copySelection(); }
  void paste() { m_iconView->iconViewWidget()->pasteSelection(); }

  void trash() { KonqOperations::del(m_iconView->iconViewWidget(),
                                     KonqOperations::TRASH,
                                     m_iconView->iconViewWidget()->selectedUrls()); }
  void del() { KonqOperations::del(m_iconView->iconViewWidget(),
                                   KonqOperations::DEL,
                                   m_iconView->iconViewWidget()->selectedUrls()); }
  void shred() { KonqOperations::del(m_iconView->iconViewWidget(),
                                     KonqOperations::SHRED,
                                     m_iconView->iconViewWidget()->selectedUrls()); }
  void properties();
  void editMimeType();

  // void print();

private:
  KonqKfmIconView *m_iconView;
  bool m_bSaveViewPropertiesLocally;
};

#endif
