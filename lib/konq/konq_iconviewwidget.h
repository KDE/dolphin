/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
    Copyright (C) 2000, 2001, 2002 David Faure <david@mandrakesoft.com>

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef __konq_iconviewwidget_h__
#define __konq_iconviewwidget_h__

#include <kiconloader.h>
#include <kiconview.h>
#include <kurl.h>
#include <qguardedptr.h>
#include <kfileitem.h>
#include <kio/jobclasses.h>

class KonqFMSettings;
class KFileIVI;
class KonqIconDrag;
namespace KIO { class Job; }

/**
 * A file-aware icon view, implementing drag'n'drop, KDE icon sizes,
 * user settings, animated icons...
 * Used by kdesktop and konq_iconview.
 *
 */
class KonqIconViewWidget : public KIconView
{
    Q_OBJECT
    Q_PROPERTY( bool sortDirectoriesFirst READ sortDirectoriesFirst WRITE setSortDirectoriesFirst )
    Q_PROPERTY( QRect iconArea READ iconArea WRITE setIconArea )
    Q_PROPERTY( int lineupMode READ lineupMode WRITE setLineupMode )
    Q_PROPERTY( QString url READ urlString WRITE setNewURL )

    friend class KFileIVI;

public:

    enum LineupMode { LineupHorizontal=1, LineupVertical, LineupBoth };

    /**
     * Constructor
     * @param settings An instance of KonqFMSettings, see static methods in konq_settings.h
     */
    KonqIconViewWidget( QWidget *parent = 0L, const char *name = 0L, WFlags f = 0, bool kdesktop = FALSE );
    virtual ~KonqIconViewWidget();

    /**
     * Read the configuration and apply it.
     * Call this in the inherited constructor with bInit=true,
     * and in some reparseConfiguration() slot with bInit=false.
     * Returns true if the font was changed (which means something has to
     * be done so that the icon's texts don't run into each other).
     * However Konq and KDesktop handle this differently.
     */
    bool initConfig( bool bInit );

    /**
     * Set the area that will be occupied by icons. It is still possible to
     * drag icons outside this area; this only applies to automatically placed
     * icons.
     */
    void setIconArea( const QRect &rect );

    /**
     * Reimplemented to make the slotOnItem highlighting work.
     */
    virtual void focusOutEvent( QFocusEvent * /* ev */ );

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
     * Line up the icons to a regular grid horizontally or vertically.
     *
     * @param isVertical
     */
    void lineupIcons( QIconView::Arrangement arrangement );

    /**
     * Sets the icons of all items, and stores the @p size
     * This doesn't touch thumbnails, except if @p stopImagePreviewFor is set.
     * Takes care of the grid, when changing the size.
     *
     * @param stopImagePreviewFor set to a list of mimetypes which should be made normal again.
     * For instance "text/plain,image/wmf".
     * Can be set to "*" for "all mimetypes" and to "image/"+"*" for "all images".
     */
    void setIcons( int size, const QStringList& stopImagePreviewFor = QStringList() );

    /**
     * Called on databaseChanged
     */
    void refreshMimeTypes();

    int iconSize() { return m_size; }

    void calculateGridX();
    /**
     * The horizontal distance between two icons
     * (whether or not a grid has been given to QIconView)
     */
    int gridXValue() const;

    /**
     * Calculate the geometry of the fixed grid that is used to line up the
     * icons, for example when using the lineupIcons() method.
     *
     * @param x
     * @param y
     * @param dx Cell width
     * @param dy Cell height
     * @param nx Number of columns
     * @param ny Number of rows
     */
    void gridValues( int* x, int* y, int* dx, int* dy, int* nx, int* ny );

    /**
     * Start generating the previews.
     * @param previewSettings
     * @param force if true, all files are looked at.
     *    Otherwise, only those which are not a thumbnail already.
     */
    void startImagePreview( const QStringList &ignored, bool force );
    void stopImagePreview();
    bool isPreviewRunning() const;
    // unused
    void setThumbnailPixmap( KFileIVI * item, const QPixmap & pixmap );
    void disableSoundPreviews();

    void setURL ( const KURL & kurl );
    // ### KDE4: make const
    const KURL & url() { return m_url; }
    QString urlString() const { return m_url.url(); }
    void setRootItem ( const KFileItem * item ) { m_rootItem = item; }

    /**
     * Get list of selected KFileItems
     */
    KFileItemList selectedFileItems();

    void setItemColor( const QColor &c );
    QColor itemColor() const;

    virtual void cutSelection();
    virtual void copySelection();
    virtual void pasteSelection();
    virtual KURL::List selectedUrls();
    void paste( const KURL &url );

    bool sortDirectoriesFirst() const;
    void setSortDirectoriesFirst( bool b );

    void setCaseInsensitiveSort( bool b );
    bool caseInsensitiveSort() const;

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

    bool maySetWallpaper();
    void setMaySetWallpaper(bool b);

    void disableIcons( const KURL::List & lst );

    QString iconPositionGroupPrefix() const { return m_iconPositionGroupPrefix; }
    QString dotDirectoryPath() const { return m_dotDirectoryPath; }

    void setPreviewSettings(const QStringList& mimeTypes);
    const QStringList& previewSettings();
    void setNewURL( const QString& url );

public slots:
    /**
     * Checks the new selection and emits enableAction() signals
     */
    virtual void slotSelectionChanged();

    void slotSaveIconPositions();

    void renameSelectedItem();
    void renameCurrentItem();

    void slotToolTipPreview( const KFileItem *, const QPixmap & );
    void slotToolTipPreviewResult();
signals:
    /**
     * For cut/copy/paste/move/delete (see kparts/browserextension.h)
     */
    void enableAction( const char * name, bool enabled );

    void dropped();
    void imagePreviewFinished();

    void incIconSize();
    void decIconSize();

    /**
     * We need to track drag in icon views for the spring loading folders
     */
    void dragEntered();
    void dragLeft();
    /**
     * Emited after the dropped() event. This way we know when the
     * drag'n'drop is really finished.
     */
    void dragFinished();

protected slots:
    virtual void slotDropped( QDropEvent *e, const QValueList<QIconDragItem> & );

    void slotItemRenamed(QIconViewItem *item, const QString &name);

    void slotIconChanged(int);
    void slotOnItem(QIconViewItem *);
    void slotOnViewport();
    void slotStartSoundPreview();
    void slotPreview(const KFileItem *, const QPixmap &);
    void slotPreviewResult();

    void slotMovieUpdate( const QRect& rect );
    void slotMovieStatus( int status );
    void slotReenableAnimation();

    void slotAboutToCreate(const QPoint &pos, const QValueList<KIO::CopyInfo> &files);
    void doubleClickTimeout();

protected:
    virtual QDragObject *dragObject();
    KonqIconDrag *konqDragObject( QWidget * dragSource = 0L );
    bool mimeTypeMatch( const QString& mimeType, const QStringList& mimeList ) const;

    virtual void drawBackground( QPainter *p, const QRect &r );
    /**
     * r is the rectangle which you want to paint from the background.
     * pt is the upper left point in the painter device where you want to paint
     * the rectangle r.
     */
    virtual void drawBackground( QPainter *p, const QRect &r,
		 			const QPoint &pt );
    virtual void contentsDragEnterEvent( QDragEnterEvent *e );
    virtual void contentsDragLeaveEvent( QDragLeaveEvent *e );
    virtual void contentsDropEvent( QDropEvent *e );
    virtual void contentsMousePressEvent( QMouseEvent *e );
    virtual void contentsMouseReleaseEvent ( QMouseEvent * e );
    virtual void contentsMouseMoveEvent( QMouseEvent *e );
    virtual void backgroundPixmapChange( const QPixmap & );
    virtual void wheelEvent( QWheelEvent* );
    void readAnimatedIconsConfig();
    void mousePressChangeValue();
    
    bool boostPreview() const;
    int previewIconSize( int size ) const;
    int largestPreviewIconSize( int size ) const;
    bool canPreview( KFileItem* item );
    void updatePreviewMimeTypes();

private:
    KURL m_url;
    const KFileItem * m_rootItem;

    KURL::List m_lstDragURLs;

    int m_size;

    /** Konqueror settings */
    KonqFMSettings * m_pSettings;

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

private:
    struct KonqIconViewWidgetPrivate *d;

};

#endif
