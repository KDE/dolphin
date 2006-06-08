/**
 * kbookmarkmerger.cpp - Copyright (C) 2005 Frerich Raabe <raabe@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <kaboutdata.h>
#include <kapplication.h>
#include <kbookmarkmanager.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>


#include <QDir>
#include <qdom.h>
#include <QFile>

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

static const KCmdLineOptions cmdLineOptions[] =
{
	{ "+directory", I18N_NOOP( "Directory to scan for extra bookmarks" ), 0 },
	KCmdLineLastOption
};

// The code for this function was taken from kdesktop/kcheckrunning.cpp
static bool kdeIsRunning()
{
#ifdef Q_WS_X11
	Display *dpy = XOpenDisplay( NULL );
	if ( !dpy ) {
		return false;
	}

	Atom atom = XInternAtom( dpy, "_KDE_RUNNING", False );
	return XGetSelectionOwner( dpy, atom ) != None;
#else
	return true;
#endif
}

int main( int argc, char**argv )
{
	const bool kdeRunning = kdeIsRunning();

	KAboutData aboutData( "kbookmarkmerger", I18N_NOOP( "KBookmarkMerger" ),
	                      "1.0", I18N_NOOP( "Merges bookmarks installed by 3rd parties into the user's bookmarks" ),
	                      KAboutData::License_BSD,
	                      I18N_NOOP(  "Copyright © 2005 Frerich Raabe" ) );
	aboutData.addAuthor( "Frerich Raabe", I18N_NOOP( "Original author" ),
	                     "raabe@kde.org" );

	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( cmdLineOptions );

	KApplication app( false );
	app.disableSessionManagement();

	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	if ( args->count() != 1 ) {
		kError() << "No directory to scan for bookmarks specified." << endl;
		return 1;
	}

	KBookmarkManager *konqBookmarks = KBookmarkManager::userBookmarksManager();
	QStringList mergedFiles;
	{
		KBookmarkGroup root = konqBookmarks->root();
		for ( KBookmark bm = root.first(); !bm.isNull(); bm = root.next( bm ) ) {
			if ( bm.isGroup() ) {
				continue;
			}

			QString mergedFrom = bm.metaDataItem( "merged_from" );
			if ( !mergedFrom.isNull() ) {
				mergedFiles << mergedFrom;
			}
		}
	}

	bool didMergeBookmark = false;

	QString extraBookmarksDirName = QFile::decodeName( args->arg( 0 ) );
	QDir extraBookmarksDir( extraBookmarksDirName, "*.xml" );
	if ( !extraBookmarksDir.isReadable() ) {
		kError() << "Failed to read files in directory " << extraBookmarksDirName << endl;
		return 1;
	}

	for ( unsigned int i = 0; i < extraBookmarksDir.count(); ++i ) {
		const QString fileName = extraBookmarksDir[ i ];
		if ( mergedFiles.contains( fileName ) ) {
			continue;
		}

		const QString absPath = extraBookmarksDir.filePath( fileName );
		KBookmarkManager *mgr = KBookmarkManager::managerForFile( absPath, false );
		KBookmarkGroup root = mgr->root();
		for ( KBookmark bm = root.first(); !bm.isNull(); bm = root.next( bm ) ) {
			if ( bm.isGroup() ) {
				continue;
			}
			bm.setMetaDataItem( "merged_from", fileName );
			konqBookmarks->root().addBookmark( konqBookmarks, bm , false );
			didMergeBookmark = true;
		}
	}

	if ( didMergeBookmark ) {
		if ( !konqBookmarks->save() ) {
			kError() << "Failed to write merged bookmarks." << endl;
			return 1;
		}
		if ( kdeRunning ) {
			konqBookmarks->notifyChanged( "" );
		}
	}
}

