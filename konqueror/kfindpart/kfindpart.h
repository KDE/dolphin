/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef kfindpart__h
#define kfindpart__h

#include <kparts/part.h>
#include <kfileitem.h>
#include <qlist.h>
class KQuery;

class KFindPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    Q_PROPERTY( bool showsResult READ showsResult )
public:
    KFindPart( QWidget * parentWidget, QObject *parent, const char *name );
    virtual ~KFindPart();

    virtual bool openURL( const KURL &url );
    virtual bool openFile() { return false; }

    bool showsResult() const { return m_bShowsResult; }

signals:
    // Konqueror connects directly to those signals
    void started(); // started a search
    void clear(); // delete all items
    void newItems(const KFileItemList&); // found this/these item(s)
    void finished(); // finished searching
    void canceled(); // the user canceled the search
    void findClosed(); // close us

protected slots:
    void slotStarted();
    void slotDestroyMe();
    void addFile(const KFileItem *item);
    void slotResult(int errorCode);

private:
    Kfind * m_kfindWidget;
    KQuery *query;
    bool m_bShowsResult; // whether the dirpart shows the results of a search or not
    /**
     * The internal storage of file items
     */
    QList<KFileItem> m_lstFileItems;

};

#endif
