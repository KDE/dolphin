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

#ifndef __konq_iconviewwidget_h__
#define __konq_iconviewwidget_h__

#include <kiconloader.h>
#include <kiconview.h>
#include <kurl.h>
#include <konq_fileitem.h>
#include <qguardedptr.h>

class KPixmapSplitter;
class KonqFMSettings;
class KFileIVI;
class KonqIconDrag;
class KonqImagePreviewJob;
namespace KIO { class Job; }

/**
 * A file-aware icon view, implementing drag'n'drop, KDE icon sizes,
 * user settings, ...
 * Used by kdesktop and konq_iconview
 */
class KonqIconViewWidget : public KIconView
{
    Q_OBJECT
    Q_PROPERTY( bool sortDirectoriesFirst READ sortDirectoriesFirst WRITE setSortDirectoriesFirst )
    Q_PROPERTY( QRect iconArea READ iconArea WRITE setIconArea )
    Q_PROPERTY( int lineupMode READ lineupMode WRITE setLineupMode )

public:

    enum LineupMode { LineupHorizontal=1, LineupVertical, LineupBoth };

    /**
     * Constructor
     * @param settings An instance of KonqFMSettings, see static methods in konq_settings.h
     */
    KonqIconViewWidget( QWidget *parent = 0L, const char *name = 0L, WFlags f = 0, bool kdesktop = FALSE );
    virtual ~KonqIconViewWidget();

    void initConfig();

    /**
     * Set the area that will be occupied by icons. It is still possible to
     * drag icons outside this area; this only applies to automatically placed
     * icons.
     */
    void setIconArea( const QRect &rect );

    /**
     * Returns the icon area.
     */
    QRect iconArea() const;

    /**
     * Set the lineup mode. This determines in which direction(s) icons are
     * moved when lineing them up.
     */
    void setLineupMode(int mode);

    /**
     * Returns the lineup mode.
     */
    int lineupMode() const;

    /**
     * Line up the icons to a regular grid. The outline of the grid is
     * specified by @ref #iconArea. The two length parameters are
     * @ref #gridX and @ref #gridY.
     */
    void lineupIcons();

    /**
     * Sets the icons of all items, and stores the @p size
     * This doesn't touch thumbnails, except @p stopImagePreview is set to true
     * Takes care of the grid, when changing the size
     */
    void setIcons( int size, bool stopImagePreview = false );

    /**
     * Called on databaseChanged
     */
    void refreshMimeTypes();

    int iconSize() { return m_size; }

    void calculateGridX();

    void startImagePreview( bool force );
    void stopImagePreview();
    void setThumbnailPixmap( KFileIVI * item, const QPixmap & pixmap );

    void setURL ( const KURL & kurl );
    const KURL & url() { return m_url; }
    void setRootItem ( const KonqFileItem * item ) { m_rootItem = item; }

    /**
     * Get list of selected KFileItems
     */
    KFileItemList selectedFileItems();

    void setItemFont( const QFont &f );
    void setItemColor( const QColor &c );
    QColor itemColor() const;

    virtual void cutSelection();
    virtual void copySelection();
    virtual void pasteSelection();
    virtual KURL::List selectedUrls();

    bool sortDirectoriesFirst() const;
    void setSortDirectoriesFirst( bool b );

    /**
     * Cache of the dragged URLs over the icon view, used by KFileIVI
     */
    const KURL::List & dragURLs() { return m_lstDragURLs; }

    /**
     * Reimplemented from QIconView
     */
    virtual void clear();

    /**
     * Reimplemented from QIconView
     */
    virtual void takeItem( QIconViewItem *item );

    /**
     * Reimplemented from QIconView to take into account @ref #iconArea.
     */
    virtual void insertInGrid( QIconViewItem *item );

    /**
     * Reimplemented from QIconView to update the gridX
     */
    virtual void setItemTextPos( ItemTextPos pos );

    /**
     * Give feedback when item is activated.
     */
    virtual void visualActivate(QIconViewItem *);

    bool isDesktop() const { return m_bDesktop; }

    /**
     * Provided for KDesktop.
     */
    virtual void setWallpaper(const KURL&) { }

    void disableIcons( const KURL::List & lst );

public slots:
    /**
     * Checks the new selection and emits enableAction() signals
     */
    virtual void slotSelectionChanged();

    void slotSaveIconPositions();

    void renameSelectedItem();

signals:
    /**
     * For cut/copy/paste/move/delete (see kparts/browserextension.h)
     */
    void enableAction( const char * name, bool enabled );

    void viewportAdjusted();
    void dropped();
    void imagePreviewFinished();

protected slots:

    virtual void slotDropped( QDropEvent *e, const QValueList<QIconDragItem> & );

    void slotItemRenamed(QIconViewItem *item, const QString &name);

    void slotIconChanged(int);
    void slotOnItem(QIconViewItem *);
    void slotOnViewport();

protected:
    virtual QDragObject *dragObject();
    KonqIconDrag *konqDragObject( QWidget * dragSource = 0L );

    virtual void drawBackground( QPainter *p, const QRect &r );
    virtual void viewportResizeEvent(QResizeEvent *);
    virtual void contentsDragEnterEvent( QDragEnterEvent *e );
    virtual void contentsDropEvent( QDropEvent *e );
    virtual void contentsMousePressEvent( QMouseEvent *e );
    virtual void contentsMouseReleaseEvent ( QMouseEvent * e );
    virtual void backgroundPixmapChange( const QPixmap & );

    KURL m_url;
    const KonqFileItem * m_rootItem;

    KURL::List m_lstDragURLs;

    int m_size;

    QGuardedPtr<KonqImagePreviewJob> m_pImagePreviewJob;

    /** Konqueror settings */
    KonqFMSettings * m_pSettings;

    KFileIVI * m_pActiveItem;
    bool m_bMousePressed;
    QPoint m_mousePos;

    QColor iColor;

    bool m_bSortDirsFirst;

    QString m_iconPositionGroupPrefix;
    QString m_dotDirectoryPath;

    int m_LineupMode;
    QRect m_IconRect;

    bool m_bDesktop;
    bool m_bSetGridX;

    /* for the PreviewJob */
    KPixmapSplitter *m_splitter;
};

#endif
