/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QStyle>
#include <QDir>
//Added by qt3to4:
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QLabel>
#include <QDropEvent>

#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kurifilter.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kwin.h>
#include <kprotocolmanager.h>
#include <kstartupinfo.h>
#include <kiconloader.h>

#include "konq_misc.h"
#include "konq_mainwindow.h"
#include "konq_viewmgr.h"
#include "konq_view.h"

/**********************************************
 *
 * KonqMisc
 *
 **********************************************/

// Terminates fullscreen-mode for any full-screen window on the current desktop
void KonqMisc::abortFullScreenMode()
{
  QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
  if ( mainWindows )
  {
    foreach ( KonqMainWindow* window, *mainWindows )
    {
      if ( window->fullScreenMode() )
      {
	KWin::WindowInfo info = KWin::windowInfo( window->winId(), NET::WMDesktop );
	if ( info.valid() && info.isOnCurrentDesktop() )
          window->showNormal();
      }
    }
  }
}

// #### this can probably be removed
KonqMainWindow * KonqMisc::createSimpleWindow( const KUrl & _url, const QString &frameName )
{
  abortFullScreenMode();

  // If _url is 0L, open $HOME [this doesn't happen anymore]
  KUrl url;
  if (_url.isEmpty())
     url.setPath(QDir::homePath());
  else
     url = _url;

  KonqMainWindow *win = new KonqMainWindow( KUrl(), false );
  win->setInitialFrameName( frameName );
  win->openURL( 0L, url );
  win->show();

  return win;
}

KonqMainWindow * KonqMisc::createSimpleWindow( const KUrl & url, const KParts::URLArgs &args, bool tempFile )
{
  abortFullScreenMode();

  KonqOpenURLRequest req;
  req.args = args;
  req.tempFile = tempFile;
  KonqMainWindow *win = new KonqMainWindow( KUrl(), false );
  win->openURL( 0L, url, QString(), req );
  win->show();

  return win;
}

KonqMainWindow * KonqMisc::createNewWindow( const KUrl &url, const KParts::URLArgs &args, bool forbidUseHTML, QStringList filesToSelect, bool tempFile, bool openURL )
{
  kDebug() << "KonqMisc::createNewWindow url=" << url << endl;

  // For HTTP or html files, use the web browsing profile, otherwise use filemanager profile
  QString profileName = (!(KProtocolManager::supportsListing(url)) ||
                        KMimeType::findByURL(url)->name() == "text/html")
          ? "webbrowsing" : "filemanagement";

  QString profile = KStandardDirs::locate( "data", QLatin1String("konqueror/profiles/") + profileName );
  return createBrowserWindowFromProfile(profile, profileName,
					url, args,
					forbidUseHTML, filesToSelect, tempFile, openURL );
}

KonqMainWindow * KonqMisc::createBrowserWindowFromProfile( const QString &path, const QString &filename, const KUrl &url, const KParts::URLArgs &args, bool forbidUseHTML, const QStringList& filesToSelect, bool tempFile, bool openURL )
{
  kDebug(1202) << "void KonqMisc::createBrowserWindowFromProfile() " << endl;
  kDebug(1202) << "path=" << path << ",filename=" << filename << ",url=" << url.prettyUrl() << endl;
  abortFullScreenMode();

  KonqMainWindow * mainWindow;
  if ( path.isEmpty() )
  {
      // The profile doesn't exit -> creating a simple window
      mainWindow = createSimpleWindow( url, args, tempFile );
      if ( forbidUseHTML )
          mainWindow->setShowHTML( false );
  }
  else if( KonqMainWindow::isPreloaded() && KonqMainWindow::preloadedWindow() != NULL )
  {
      mainWindow = KonqMainWindow::preloadedWindow();
#ifdef Q_WS_X11
      KStartupInfo::setWindowStartupId( mainWindow->winId(), kapp->startupId());
#endif
      KonqMainWindow::setPreloadedWindow( NULL );
      KonqMainWindow::setPreloadedFlag( false );
      mainWindow->resetWindow();
      mainWindow->reparseConfiguration();
      if( forbidUseHTML )
          mainWindow->setShowHTML( false );
      KonqOpenURLRequest req;
      req.args = args;
      req.filesToSelect = filesToSelect;
      req.tempFile = tempFile;
      mainWindow->viewManager()->loadViewProfile( path, filename, url, req, true );
  }
  else
  {
      KConfig cfg( path, true );
      cfg.setDollarExpansion( true );
      cfg.setGroup( "Profile" );
      QString xmluiFile=cfg.readEntry("XMLUIFile","konqueror.rc");

      mainWindow = new KonqMainWindow( KUrl(), false, xmluiFile );
      if ( forbidUseHTML )
          mainWindow->setShowHTML( false );
      KonqOpenURLRequest req;
      req.args = args;
      req.filesToSelect = filesToSelect;
      req.tempFile = tempFile;
      mainWindow->viewManager()->loadViewProfile( cfg, filename, url, req, false, openURL );
  }
  mainWindow->setInitialFrameName( args.frameName );
  mainWindow->show();
  return mainWindow;
}

KonqMainWindow * KonqMisc::newWindowFromHistory( KonqView* view, int steps )
{
  int oldPos = view->historyIndex();
  int newPos = oldPos + steps;

  const HistoryEntry * he = view->historyAt(newPos);
  if(!he)
      return 0L;

  KonqMainWindow* mainwindow = createNewWindow(he->url, KParts::URLArgs(),
					       false, QStringList(), false, /*openURL*/false);
  if(!mainwindow)
      return 0L;
  KonqView* newView = mainwindow->currentView();

  if(!newView)
      return 0L;

  newView->copyHistory(view);
  newView->setHistoryIndex(newPos);
  newView->restoreHistory();
  return mainwindow;
}

QString KonqMisc::konqFilteredURL( QWidget* parent, const QString& _url, const QString& _path )
{
  if ( !_url.startsWith( "about:" ) ) // Don't filter "about:" URLs
  {
    KURIFilterData data = _url;

    if( !_path.isEmpty() )
      data.setAbsolutePath(_path);

    // We do not want to the filter to check for executables
    // from the location bar.
    data.setCheckForExecutables (false);

    if( KURIFilter::self()->filterURI( data ) )
    {
      if( data.uriType() == KURIFilterData::ERROR && !data.errorMsg().isEmpty() )
      {
        KMessageBox::sorry( parent, i18n( data.errorMsg().toUtf8() ) );
        return QString();
      }
      else
        return data.uri().url();
    }
  }
  else if ( _url.startsWith( "about:" ) && _url != "about:blank" ) {
    // We can't use "about:" as it is, KUrl doesn't parse it.
    if (_url == "about:plugins")
       return "about:plugins";
    return "about:konqueror";
  }
  return _url;  // return the original url if it cannot be filtered.
}

KonqDraggableLabel::KonqDraggableLabel( KonqMainWindow* mw, const QString& text )
  : QLabel( text )	// Use this name for it to be styled!
  , m_mw(mw)
{
  setObjectName( "kde toolbar widget" );
  setBackgroundRole( QPalette::Button );
  setAlignment( (QApplication::isRightToLeft() ? Qt::AlignRight : Qt::AlignLeft) |
                 Qt::AlignVCenter | Qt::TextShowMnemonic );
  setAcceptDrops(true);
  adjustSize();
  validDrag = false;
}

void KonqDraggableLabel::mousePressEvent( QMouseEvent * ev )
{
  validDrag = true;
  startDragPos = ev->pos();
}

void KonqDraggableLabel::mouseMoveEvent( QMouseEvent * ev )
{
  if ((startDragPos - ev->pos()).manhattanLength() > QApplication::startDragDistance())
  {
    validDrag = false;
    if ( m_mw->currentView() )
    {
      KUrl::List lst;
      lst.append( m_mw->currentView()->url() );
      QDrag* drag = new QDrag( m_mw );
      QMimeData* md = new QMimeData();
      lst.populateMimeData( md );
      drag->setMimeData( md );
      QString iconName = KMimeType::iconNameForURL( lst.first() );

      drag->setPixmap(KGlobal::iconLoader()->loadMimeTypeIcon(iconName, K3Icon::Small));

      drag->start();
    }
  }
}

void KonqDraggableLabel::mouseReleaseEvent( QMouseEvent * )
{
  validDrag = false;
}

void KonqDraggableLabel::dragEnterEvent( QDragEnterEvent *ev )
{
  if ( KUrl::List::canDecode( ev->mimeData() ) )
    ev->accept();
}

void KonqDraggableLabel::dropEvent( QDropEvent* ev )
{
  _savedLst.clear();
  _savedLst = KUrl::List::fromMimeData( ev->mimeData() );
  if ( !_savedLst.isEmpty() ) {
    QTimer::singleShot(0, this, SLOT(delayedOpenURL()));
  }
}

void KonqDraggableLabel::delayedOpenURL()
{
    m_mw->openURL( 0L, _savedLst.first() );
}

#include "konq_misc.moc"
