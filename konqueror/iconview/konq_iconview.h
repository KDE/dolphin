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

#include <kbrowser.h>

class KonqKfmIconView;
class KonqIconViewWidget;
class KonqPropsView;
class KDirLister;
class KFileItem;
class KonqFMSettings;
class KFileIVI;
class KAction;
class QActionMenu;
class QIconViewItem;
class IconEditExtension; // defined in libkonq/konqiconviewwidget.h

class IconViewPropertiesExtension : public ViewPropertiesExtension
{
  Q_OBJECT
public:
  IconViewPropertiesExtension( KonqKfmIconView *iconView );

  virtual void reparseConfiguration();
  virtual void saveLocalProperties();
  virtual void savePropertiesAsDefault();

private:
  KonqKfmIconView *m_iconView;  
};

/**
 * The Icon View for konqueror. Handles big icons (Horizontal mode) and
 * small icons (Vertical mode).
 * The "Kfm" in the name stands for file management since it shows files :)
 */
class KonqKfmIconView : public BrowserView
{
  friend class IconViewPropertiesExtension;
  Q_OBJECT
public:

  enum SortCriterion { NameCaseSensitive, NameCaseInsensitive, Size };

  KonqKfmIconView();
  virtual ~KonqKfmIconView();

  virtual void openURL( const QString &url, bool reload = false,
                        int xOffset = 0, int yOffset = 0 );

  virtual QString url();
  virtual int xOffset();
  virtual int yOffset();
  virtual void stop();

  virtual void saveState( QDataStream &stream );
  virtual void restoreState( QDataStream &stream );

  KonqIconViewWidget *iconViewWidget() const { return m_pIconView; }

public slots:
  void slotImagePreview( bool toggle );
  void slotShowDot();
  void slotSelect();
  void slotUnselect();
  void slotSelectAll();
  void slotUnselectAll();

  void slotSortByNameCaseSensitive( bool toggle );
  void slotSortByNameCaseInsensitive( bool toggle );
  void slotSortBySize( bool toggle );
  void slotSortDescending( bool toggle );

  void slotKofficeMode( bool b );
  void slotViewLarge( bool b );
  void slotViewNormal( bool b );
  void slotViewSmall( bool b );

  void slotTextBottom( bool b );
  void slotTextRight( bool b );

  void slotBackgroundColor();
  void slotBackgroundImage();

protected slots:
  // slots connected to QIconView
  void slotReturnPressed( QIconViewItem *item );
  void slotMouseButtonPressed(int, QIconViewItem*, const QPoint&);
  void slotViewportRightClicked();
  void slotOnItem( QIconViewItem *item );
  void slotOnViewport();

  // slots connected to the directory lister
  void slotStarted( const QString & );
  void slotCompleted();
  void slotNewItem( KFileItem * );
  void slotDeleteItem( KFileItem * );

  void slotClear();

  void slotTotalFiles( int, unsigned long files );

protected:

  void setupSorting( SortCriterion criterion );

  virtual void resizeEvent( QResizeEvent * );

  /** */
  void setupSortKeys();

  QString makeSizeKey( KFileIVI *item );

  /** The directory lister for this URL */
  KDirLister* m_dirLister;

  /** View properties */
  KonqPropsView * m_pProps;

  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit;

  bool m_bLoading;
  /**
   * Set to true if slotCompleted needs to realign the icons
   */
  bool m_bNeedAlign;

  int m_iXOffset;
  int m_iYOffset;

  unsigned long m_ulTotalFiles;

  SortCriterion m_eSortCriterion;

  KToggleAction *m_paDotFiles;
  KToggleAction *m_paImagePreview;
  KActionMenu *m_pamSort;

  KToggleAction *m_paLargeIcons;
  KToggleAction *m_paNormalIcons;
  KToggleAction *m_paSmallIcons;

  KToggleAction *m_paBottomText;
  KToggleAction *m_paRightText;

  KToggleAction *m_paKOfficeMode;
  KAction *m_paSelect;
  KAction *m_paUnselect;
  KAction *m_paSelectAll;
  KAction *m_paUnselectAll;

  long m_lDirSize;
  long m_lFileCount;
  long m_lDirCount;

  IconEditExtension *m_extension;

  KonqIconViewWidget *m_pIconView;
};


#endif
