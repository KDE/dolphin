/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konqdirpart_h
#define __konqdirpart_h

#include <qstring.h>
#include <kparts/part.h>
#include <kfileitem.h>

namespace KParts { class BrowserExtension; }
class KonqPropsView;
class QScrollView;
class KAction;
class KToggleAction;

class KonqDirPart: public KParts::ReadOnlyPart
{
  Q_OBJECT
public:
    KonqDirPart( QObject *parent, const char *name );
    virtual ~KonqDirPart();

    /**
     * The derived part should call this in its constructor
     */
    void setBrowserExtension( KParts::BrowserExtension * extension )
      { m_extension = extension; }
    KParts::BrowserExtension * extension()
      { return m_extension; }

    QScrollView * scrollWidget();

    void saveNameFilter( QDataStream &stream );
    void restoreNameFilter( QDataStream &stream );
    virtual void saveState( QDataStream &stream );
    virtual void restoreState( QDataStream &stream );

    void mmbClicked( KFileItem * fileItem );

    void setNameFilter( const QString & nameFilter ) { m_nameFilter = nameFilter; }
    QString nameFilter() const { return m_nameFilter; }

    /**
     * Sets per directory mime-type based filtering.
     *
     * This method causes only the items matching the mime-type given
     * by @p filters to be displayed. You can supply multiple mime-types
     * by separating them with a space, eg. "text/plain image/x-png".
     * To clear all the filters set for the current url simply call this
     * function with a null or empty argument.
     *
     * NOTE: the filter(s) specified here only apply to the current
     * directory as returned by @ref #url().
     *
     * @param filter mime-type(s) to filter directory by.
     */
    void setMimeFilter( const QString& filters );

    /**
     * Completely clears the internally stored list of mime filters
     * set by call to @ref #setMimeFilter.
     */
    QString mimeFilter() const;


    KonqPropsView * props() const { return m_pProps; }

    /**
     * "Cut" icons : disable those whose URL is in lst, enable the others
     */
    virtual void disableIcons( const KURL::List & lst ) = 0;

    /**
     * This class takes care of the counting of items, size etc. in the
     * current directory. Call this in slotClear.
     */
    void resetCount()
    {
        m_lDirSize = 0;
        m_lFileCount = 0;
        m_lDirCount = 0;
    }
    /**
     * Update the counts for those new items
     */
    void newItems( const KFileItemList & entries );
    /**
     * Update the counts with this item being deleted
     */
    void deleteItem( KFileItem * fileItem );
    /**
     * Show the counts for the directory in the status bar
     */
    void emitTotalCount();
    /**
     * Show the counts for the selected items in the status bar, if any
     * otherwise show the info for the directory.
     * @param selectionChanged if true, we'll emit selectionInfo.
     */
    void emitCounts( const KFileItemList & lst, bool selectionChanged );

    /**
     * Enables or disables the paste action. This depends both on
     * the data in the clipboard and the number of files selected
     * (pasting is only possible if not more than one file is selected).
     */
    void updatePasteAction();

    /**
     * Change the icon size of the view.
     * The view should call it initially.
     * The view should also reimplement it, to update the icons.
     */
    virtual void newIconSize( int size );

    /**
     * This is called by the actions that change the icon size.
     * It stores the new size and calls newIconSize.
     */
    void setIconSize( int size );

    /**
     * This is called by konqueror itself, when the "find" functionality is activated
     */
    void setFindPart( KParts::ReadOnlyPart * part );

    KParts::ReadOnlyPart * findPart() const { return m_findPart; }

    virtual const KFileItem * currentItem() = 0; // { return 0L; }

    virtual KFileItemList selectedFileItems() { return KFileItemList(); }

signals:
    /**
     * We emit this if we want a find part to be created for us.
     * This happens when restoring from history
     */
    void findOpen( KonqDirPart * );
    /**
     * We emit this _after_ a find part has been created for us.
     * This also happens initially.
     */
    void findOpened( KonqDirPart * );

    /**
     * We emit this to ask konq to close the find part
     */
    void findClosed( KonqDirPart * );

    /**
     * Emitted as the part is updated with new items.
     * Useful for informing plugins of changes in view.
     */
    void itemsAdded( const KFileItemList& );

    /**
     * Emitted as the part is updated with these items.
     * Useful for informing plugins of changes in view.
     */
    void itemRemoved( const KFileItem* );

    /**
     * Emitted with the list of filtered-out items whenever
     * a mime-based filter(s) is set.
     */
    void itemsFilteredByMime( const KFileItemList& );

public slots:

    /**
     * This is called either by the part's close button, or by the dir part
     * itself, if entering a directory. It deletes the find part !
     */
    void slotFindClosed();
    /* Start and stop the animated "K" during
        kfindpart's file search
    */
    void slotStartAnimationSearching();
    void slotStopAnimationSearching();

    void slotBackgroundColor();
    void slotBackgroundImage();
    /**
     * Called when the clipboard's data changes, to update the 'cut' icons
     * Call this when the directory's listing is finished, to draw icons as cut.
     */
    void slotClipboardDataChanged();

    void slotIncIconSize();
    void slotDecIconSize();

    void slotIconSizeToggled( bool );

    // slots connected to the directory lister - or to the kfind interface
    virtual void slotStarted() = 0;
    virtual void slotCanceled() = 0;
    virtual void slotCompleted() = 0;
    virtual void slotNewItems( const KFileItemList& ) = 0;
    virtual void slotDeleteItem( KFileItem * ) = 0;
    virtual void slotRefreshItems( const KFileItemList& ) = 0;
    virtual void slotClear() = 0;
    virtual void slotRedirection( const KURL & ) = 0;

protected:

    /**
     * Call this at the beginning of openURL
     */
    void beforeOpenURL();

    QString m_nameFilter;

    KParts::BrowserExtension * m_extension;
    /**
     * View properties
     */
    KonqPropsView * m_pProps;

    KAction *m_paIncIconSize;
    KAction *m_paDecIconSize;
    KToggleAction *m_paDefaultIcons;
    KToggleAction *m_paHugeIcons;
    KToggleAction *m_paLargeIcons;
    KToggleAction *m_paMediumIcons;
    KToggleAction *m_paSmallIcons;

    KParts::ReadOnlyPart * m_findPart;

    int m_iIconSize[5];

    long long m_lDirSize;
    uint m_lFileCount;
    uint m_lDirCount;
    //bool m_bMultipleItemsSelected;

private:
    class KonqDirPartPrivate;
    KonqDirPartPrivate* d;
};

#endif
