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
#include <konqfileitem.h>

class KonqFMSettings;
class KFileIVI;
class KonqIconDrag;
namespace KIO { class Job; }

/**
 * A file-aware icon view, implementing drag'n'drop, KDE icon sizes,
 * user settings, ...
 * Used by kdesktop and konq_iconview
 */
class KonqIconViewWidget : public KIconView
{
    Q_OBJECT
    Q_PROPERTY( bool sortDirectoriesFirst READ sortDirectoriesFirst WRITE setSortDirectoriesFirst );
    Q_PROPERTY( QRect iconArea READ iconArea WRITE setIconArea );
    Q_PROPERTY( int lineupMode READ lineupMode WRITE setLineupMode );

public:

    enum LineupMode { LineupHorizontal=1, LineupVertical, LineupBoth };

    /**
     * Constructor
     * @param settings An instance of KonqFMSettings, see static methods in konqsettings.h
     */
    KonqIconViewWidget( QWidget *parent = 0L, const char *name = 0L, WFlags f = 0 );
    virtual ~KonqIconViewWidget() {}

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
     */
    void setIcons( int size );

    /**
     * Called on databaseChanged
     */
    void refreshMimeTypes();

    int iconSize() { return m_size; }

    void setImagePreviewAllowed( bool b );

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
     * Give feedback when item is activated.
     */
    virtual void visualActivate(QIconViewItem *);

public slots:
    /**
     * Called when the clipboard's data changes, to update the 'cut' icons
     * Call this when the directory's listing is finished, to draw icons as cut.
     */
    virtual void slotClipboardDataChanged();
    /**
     * Checks the new selection and emits enableAction() signals
     */
    virtual void slotSelectionChanged();

    void slotSaveIconPositions();

signals:
    /**
     * For cut/copy/paste/move/delete (see kparts/browserextension.h)
     */
    void enableAction( const char * name, bool enabled );
    void viewportAdjusted();
    void dropped();

protected slots:

    virtual void slotViewportScrolled(int);

    virtual void slotDropped( QDropEvent *e, const QValueList<QIconDragItem> & );
    /** connect each item to this */
    virtual void slotDropItem( KFileIVI *item, QDropEvent *e );

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

    KURL m_url;
    const KonqFileItem * m_rootItem;

    KURL::List m_lstDragURLs;

    int m_size;
    bool m_bImagePreviewAllowed;

    /** Konqueror settings */
    KonqFMSettings * m_pSettings;

    KFileIVI * m_pActiveItem;
    bool m_bMousePressed;

    QColor iColor;

    bool m_bSortDirsFirst;

    QString m_iconPositionGroupPrefix;
    QString m_dotDirectoryPath;

    int m_LineupMode;
    QRect m_IconRect;
};

#endif
