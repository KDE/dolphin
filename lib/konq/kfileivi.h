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

#ifndef __kfileivi_h__
#define __kfileivi_h__ $Id$

#include <qiconview.h>
#include <kiconloader.h>

class KonqFileItem;

/**
 * KFileIVI (short form of "Konq - File - IconViewItem")
 * is, as expected, an improved QIconViewItem, because
 * it represents a file.
 * All the information about the file is contained in the KonqFileItem
 * pointer.
 */
class KFileIVI : public QObject, public QIconViewItem
{
    Q_OBJECT
public:
    /**
     * Create an icon, within a qlistview, representing a file
     * @param parent the parent widget
     * @param fileitem the file item created by KDirLister
     * @param size the icon size
     * @param bImagePreviewAllowed whether image preview is allowed, see KonqFileItem
     */
    KFileIVI( QIconView *iconview, KonqFileItem* fileitem, int size, bool bImagePreviewAllowed );
    virtual ~KFileIVI() { }

    /**
     * Handler for return (or single/double click) on ONE icon.
     * Runs the file through KRun.
     */
    virtual void returnPressed();

    /** @return the file item held by this instance */
    KonqFileItem * item() { return m_fileitem; }

    /** @return true if dropping on this file is allowed
     * Overloads QIconView::acceptDrop() */
    virtual bool acceptDrop( const QMimeSource *mime ) const;

    /** */
    virtual void setIcon( int size, bool bImagePreviewAllowed );

    virtual void setKey( const QString &key );
    virtual void paintItem( QPainter *p, const QColorGroup &cg );

signals:
    void dropMe( KFileIVI *item, QDropEvent *e );

protected:
    virtual void dropped( QDropEvent *e, const QValueList<QIconDragItem> &  );

    int m_size;
    bool m_bpreview;
    /** Pointer to the file item in KDirLister's list */
    KonqFileItem* m_fileitem;

private slots:
    void slotIconChanged(int);

};

#endif
