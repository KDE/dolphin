/* This file is part of the KDE project
   Copyright (C) 1999 Kurt Granroth <granroth@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef KBOOKMARKBAR_H
#define KBOOKMARKBAR_H

#include <qobject.h>
#include <qlist.h>

class KToolBar;
class KBookmark;
class KBookmarkOwner;
class KBookmarkMenu;
class KActionCollection;
class KAction;

/**
 * This class provides a bookmark toolbar.  Using this class is nearly
 * identical to using @ref KBookmarkMenu so follow the directions
 * there.
 */
class KBookmarkBar : public QObject
{
    Q_OBJECT
public:
    /**
     * Fills a bookmark toolbar
     *
     * @param _owner implementation of the KBookmarkOwner interface (callbacks)
     * @param _toolBar toolbar to fill
     * @param _collec parent for the KActions
     */
        KBookmarkBar( KBookmarkOwner *_owner, KToolBar *_toolBar,
                  KActionCollection *_collec, QObject *parent = 0L, const char *name = 0L);
        virtual ~KBookmarkBar();

public slots:
    void slotBookmarksChanged();
    void slotBookmarkSelected();
    void clear();

protected:
    void fillBookmarkBar( KBookmark *parent );

    KBookmarkOwner    *m_pOwner;
    KToolBar          *m_toolBar;
    KActionCollection *m_actionCollection;
    QList<KAction>     m_actions;
    QList<KBookmarkMenu> m_lstSubMenus;
};

#endif // KBOOKMARKBAR_H
