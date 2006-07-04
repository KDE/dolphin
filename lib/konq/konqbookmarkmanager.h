#ifndef KONQBOOKMARKMANAGER_H
#define KONQBOOKMARKMANAGER_H

#include <kbookmarkmanager.h>
#include <kstandarddirs.h>
#include <libkonq_export.h>

class LIBKONQ_EXPORT KonqBookmarkManager
{
public:
    static KBookmarkManager * self() {
        if ( !s_bookmarkManager )
        {
            QString bookmarksFile = KStandardDirs::locateLocal("data", QLatin1String("konqueror/bookmarks.xml"));
            s_bookmarkManager = KBookmarkManager::managerForFile( bookmarksFile, "konqueror" );
        }
        return s_bookmarkManager;
    }

private:
    static KBookmarkManager *s_bookmarkManager;
};

#endif
