#ifndef KONQBOOKMARKMANAGER_H
#define KONQBOOKMARKMANAGER_H

#include <kbookmarkmanager.h>
#include <kstandarddirs.h>

class KonqBookmarkManager
{
public:
    static KBookmarkManager * self() {
        if ( !s_bookmarkManager )
        {
            QString bookmarksFile = locateLocal("data", QString::fromLatin1("konqueror/bookmarks.xml"));
            s_bookmarkManager = KBookmarkManager::self( bookmarksFile );
        }
        return s_bookmarkManager;
    }


    static KBookmarkManager *s_bookmarkManager;
};

#endif
