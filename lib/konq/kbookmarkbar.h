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
#include <qguardedptr.h>
#include <qlist.h>
#include <kbookmark.h>

class KToolBar;
class KBookmarkMenu;
class KBookmarkOwner;
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
     * @param owner implementation of the KBookmarkOwner interface (callbacks)
     * @param toolBar toolbar to fill
     * @param collec parent for the KActions OBSOLETE
     * @param manager the bookmark manager
     */
    KBookmarkBar( KBookmarkOwner *owner, KToolBar *toolBar,
                  KActionCollection *,
                  QObject *parent = 0L, const char *name = 0L);

    virtual ~KBookmarkBar();

public slots:
    void slotBookmarksChanged( const QString & );
    void slotBookmarkSelected();
    void clear();

protected:
    void fillBookmarkBar( KBookmarkGroup & parent );

private:
    KBookmarkOwner    *m_pOwner;
    QGuardedPtr<KToolBar> m_toolBar;
    KActionCollection *m_actionCollection;
    QList<KAction>     m_actions;
    QList<KBookmarkMenu> m_lstSubMenus;
};

#endif // KBOOKMARKBAR_H
