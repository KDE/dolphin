/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id: kfileivi.h,v 1.23 2000/06/29 19:45:34 faure Exp

#ifndef __kfileivi_h__
#define __kfileivi_h__

#include <kiconview.h>
#include <kiconloader.h>
#include <konq_fileitem.h>
class KonqIconViewWidget;

/**
 * KFileIVI (short form of "Konq - File - IconViewItem")
 * is, as expected, an improved QIconViewItem, because
 * it represents a file.
 * All the information about the file is contained in the KonqFileItem
 * pointer.
 */
class KFileIVI : public QIconViewItem
{
public:
    /**
     * Create an icon, within a qlistview, representing a file
     * @param parent the parent widget
     * @param fileitem the file item created by KonqDirLister
     * @param size the icon size
     */
    KFileIVI( KonqIconViewWidget *iconview, KonqFileItem* fileitem, int size );
    virtual ~KFileIVI();

    /**
     * Handler for return (or single/double click) on ONE icon.
     * Runs the file through KRun.
     */
    virtual void returnPressed();

    /**
     * @return the file item held by this instance
     */
    KonqFileItem * item() { return m_fileitem; }

    /**
     * @return true if dropping on this file is allowed
     * Overloads QIconView::acceptDrop()
     */
    virtual bool acceptDrop( const QMimeSource *mime ) const;

    /**
     * Changes the icon for this item.
     * @param size the icon size (0 for default, otherwise size in pixels)
     * @param state the state of the icon (enum in KIcon)
     * @param recalc whether to update the layout of the icon view when setting the icon
     * @param redraw whether to redraw the item after setting the icon
     */
    virtual void setIcon( int size,
                          int state=KIcon::DefaultState,
                          bool recalc=false,
                          bool redraw=false);

    /**
     * Notifies that all icon effects on thumbs should be invalidated,
     * e.g. because the effect settings have been changed. The thumb itself
     * is assumed to be still valid (use setThumbnailPixmap() instead
     * otherwise).
     * @param state the state of the icon (enum in KIcon)
     * @param redraw whether to redraw the item after setting the icon
     */
    void invalidateThumb( int state, bool redraw = false );

    /**
     * Return the current state of the icon
     * (KIcon::DefaultState, KIcon::ActiveState etc.)
     */
    int state() { return m_state; }

    /**
     * Set to true when this icon is 'cut'
     */
    void setDisabled( bool disabled );

    /**
     * Set this when the thumbnail was loaded
     */
    void setThumbnailPixmap( const QPixmap & pixmap );

    /**
     * Set the icon to use the specified KIconEffect
     * See the docs for KIconEffect for details.
     */
    void setEffect( int group, int state );
    
    /**
     * @return true if this item is a thumbnail
     */
    bool isThumbnail() const { return m_bThumbnail; }

    /**
     * @deprecated
     * The desktop entry name of the thumbnail creator that was
     * used to generate this item's thumbnail
     * ### KDE 3.0 remove
     */
    void setThumbnailName( const QString & name );

    /**
     * @deprecated
     * Returns the thumbnail name as set by @ref setThumbnailName()
     * ### KDE 3.0 remove
     */
    const QString &thumbnailName() const { return m_thumbnailName; }

    /**
     * Redetermines the icon (useful if KFileItem might return another icon).
     * Does nothing with thumbnails
     */
    virtual void refreshIcon( bool redraw );

    virtual void setKey( const QString &key );

    /**
     * Paints this item. Takes care of using the normal or alpha
     * blending methods depending on the configuration.
     */
    virtual void paintItem( QPainter *p, const QColorGroup &cg );

    /**
     * Paints this item using the KAlphaPainter methods.
     * @p bgx and @p bgy specify the coordinates where the background
     * starts so that you don't have to use the whole background as
     * third parameter. copyOnBg specify if you want to get a copy of
     * the alpha blended item on the background image, or you prefer
     * not to change it (default).
     *
     * Note that when using copyOnBg=true, it won't paint on the
     * painter object, so you can use p=0L.
     */
    void paintAlphaItem( QPainter *p, const QColorGroup &cg,
	QImage &background, QPixmap &unknown_icon,
	bool copyOnBg=false, int bgx=0, int bgy=0);

    void move( int x, int y );

protected:
    virtual void dropped( QDropEvent *e, const QValueList<QIconDragItem> &  );

private:
    int m_size, m_state;
    bool m_bDisabled;
    bool m_bThumbnail;
    QString m_thumbnailName; // ### KDE 3.0 remove
    /** Pointer to the file item in KonqDirLister's list */
    KonqFileItem* m_fileitem;
    
    /**
     * Private data for KFileIVI
     * Implementation in kfileivi.cc
     */
    struct Private;
    
    Private *d;
};

#endif
