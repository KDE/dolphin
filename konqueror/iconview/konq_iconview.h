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

#include "browser.h"

#include <qiconview.h>
#include <qtimer.h>
#include <qstrlist.h>

class KonqKfmIconView;
class KonqIconViewWidget;
class KonqPropsView;
class KDirLister;
class KFileItem;
class KonqSettings;
class KFileIVI;
class KAction;
class QActionMenu;

class KonqDragItem : public QIconDragItem
{
public:
    KonqDragItem();
    KonqDragItem( const QRect &ir, const QRect &tr, const QString &u );
    ~KonqDragItem();
	
    QString url() const;
    void setURL( const QString &u );

protected:
    void makeKey();

    QString url_;

};

/*****************************************************************************
 *
 * Class KonqDrag
 *
 *****************************************************************************/

class KonqDrag : public QIconDrag
{
    Q_OBJECT

public:
    typedef QValueList<KonqDragItem> KonqList;

    KonqDrag( QWidget * dragSource, const char* name = 0 );
    ~KonqDrag();

    const char* format( int i ) const;
    QByteArray encodedData( const char* mime ) const;

    void append( const KonqDragItem &icon_ );

    static bool canDecode( QMimeSource* e );

    static bool decode( QMimeSource *e, QValueList<KonqDragItem> &list_ );
    static bool decode( QMimeSource *e, QStringList &uris );

protected:
    KonqList icons;

};

class IconEditExtension : public EditExtension
{
  friend class KonqKfmIconView;
  Q_OBJECT
public:
  IconEditExtension( KonqKfmIconView *iconView );
  
  virtual void can( bool &copy, bool &paste, bool &move );

  virtual void copySelection();
  virtual void pasteSelection();
  virtual void moveSelection( const QString &destinationURL = QString::null );
  
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

  KonqIconViewWidget *iconView() const { return m_pIconView; }

public slots:
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

protected slots:
  // slots connected to QIconView
  virtual void slotMousePressed( QIconViewItem *item );
  virtual void slotDrop( QDropEvent *e );
  void slotDropItem( KFileIVI *item, QDropEvent *e );

  void slotItemRightClicked( QIconViewItem *item );
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
  /** Common to slotDrop and slotDropItem */
  void dropStuff( QDropEvent *e, KFileIVI *item = 0L );

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

  int m_iXOffset;
  int m_iYOffset;

  unsigned long m_ulTotalFiles;

  long int m_idShowDotFiles;
  long int m_idSortByNameCaseSensitive;
  long int m_idSortByNameCaseInsensitive;
  long int m_idSortBySize;
  long int m_idSortDescending;

  SortCriterion m_eSortCriterion;

  KToggleAction *m_paDotFiles;
  KActionMenu *m_pamSort;

  KToggleAction *m_paKOfficeMode;
  KAction *m_paSelect;
  KAction *m_paUnselect;
  KAction *m_paSelectAll;
  KAction *m_paUnselectAll;

  IconEditExtension *m_extension;

  KonqIconViewWidget *m_pIconView;
};

class KonqIconViewWidget : public QIconView
{
public:
  KonqIconViewWidget( KonqPropsView *props, 
             QWidget *parent = 0L, const char *name = 0L, WFlags f = 0 )
  : QIconView( parent, name, f ), m_pProps( props ) {}

  virtual QDragObject *dragObject();

  void initConfig();

protected:
  virtual void drawBackground( QPainter *p, const QRect &r );

  void initDrag( QDropEvent *e );

  KonqPropsView * m_pProps;
  /** Konqueror settings */
  KonqSettings * m_pSettings;
};

#endif
