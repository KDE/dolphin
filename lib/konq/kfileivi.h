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

class KFileItem;
class KonqIconViewWidget;

/**
 * KFileIVI (short form of "Konq - File - IconViewItem")
 * is, as expected, an improved KIconViewItem, because
 * it represents a file.
 * All the information about the file is contained in the KFileItem
 * pointer.
 */
class KFileIVI : public KIconViewItem
{
public:
    /**
     * Create an icon, within a qlistview, representing a file
     * @param parent the parent widget
     * @param fileitem the file item created by KDirLister
     * @param size the icon size
     */
    KFileIVI( KonqIconViewWidget *iconview, KFileItem* fileitem, int size );
    virtual ~KFileIVI();

    /**
     * Handler for return (or single/double click) on ONE icon.
     * Runs the file through KRun.
     */
    virtual void returnPressed();

    /**
     * @return the file item held by this instance
     */
    KFileItem * item() const { return m_fileitem; }

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
    int state() const { return m_state; }

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

    bool move( int x, int y );

protected:
    virtual void dropped( QDropEvent *e, const QValueList<QIconDragItem> &  );

private:
    /** You are not supposed to call this on a KFileIVI, from the outside,
     * it bypasses the icons cache */
    virtual void setPixmap ( const QPixmap & icon ) { KIconViewItem::setPixmap( icon ); }
    virtual void setPixmap ( const QPixmap & icon, bool recalc, bool redraw = TRUE )
        { KIconViewItem::setPixmap( icon, recalc, redraw ); }
    int m_size, m_state;
    bool m_bDisabled;
    bool m_bThumbnail;
    /** Pointer to the file item in KDirLister's list */
    KFileItem* m_fileitem;

    /**
     * Private data for KFileIVI
     * Implementation in kfileivi.cc
     */
    struct Private;

    Private *d;
};

#endif
