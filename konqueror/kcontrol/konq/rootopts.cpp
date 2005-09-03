//
//
// "Desktop Options" Tab for KDesktop configuration
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
//
// Layouts
// (c) Christian Tibirna 1998
// Port to KControl, split from Misc Tab, Port to KControl2
// (c) David Faure 1998
// Desktop menus, paths
// (c) David Faure 2000

#include <config.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>

//Added by qt3to4:
#include <QGridLayout>
#include <Q3CString>
#include <QDesktopWidget>

#include <dcopclient.h>

#include <kapplication.h>
#include <kcustommenueditor.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kglobalsettings.h>
#include <kipc.h>
#include <klistview.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <ktrader.h>
#include <konq_defaults.h> // include default values directly from libkonq
#include <kurlrequester.h>

#include "rootopts.h"

//-----------------------------------------------------------------------------

DesktopPathConfig::DesktopPathConfig(QWidget *parent, const char * )
    : KCModule( parent, "kcmkonq" )
{
  QLabel * tmpLabel;

#undef RO_LASTROW
#undef RO_LASTCOL
#define RO_LASTROW 4   // 3 paths + last row
#define RO_LASTCOL 2

  int row = 0;
  QGridLayout *lay = new QGridLayout(this, RO_LASTROW+1, RO_LASTCOL+1,
      0, KDialog::spacingHint());

  lay->setRowStretch(RO_LASTROW,10); // last line grows

  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,10);


  setQuickHelp( i18n("<h1>Paths</h1>\n"
    "This module allows you to choose where in the filesystem the "
    "files on your desktop should be stored.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options."));

  // Desktop Paths
  row++;
  tmpLabel = new QLabel(i18n("Des&ktop path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urDesktop = new KURLRequester(this);
  urDesktop->setMode( KFile::Directory );
  tmpLabel->setBuddy( urDesktop );
  lay->addMultiCellWidget(urDesktop, row, row, 1, RO_LASTCOL);
  connect(urDesktop, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  QString wtstr = i18n("This folder contains all the files"
                       " which you see on your desktop. You can change the location of this"
                       " folder if you want to, and the contents will move automatically"
                       " to the new location as well.");
  tmpLabel->setWhatsThis( wtstr );
  urDesktop->setWhatsThis( wtstr );

  row++;
  tmpLabel = new QLabel(i18n("A&utostart path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urAutostart = new KURLRequester(this);
  urAutostart->setMode( KFile::Directory );
  tmpLabel->setBuddy( urAutostart );
  lay->addMultiCellWidget(urAutostart, row, row, 1, RO_LASTCOL);
  connect(urAutostart, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This folder contains applications or"
               " links to applications (shortcuts) that you want to have started"
               " automatically whenever KDE starts. You can change the location of this"
               " folder if you want to, and the contents will move automatically"
               " to the new location as well.");
  tmpLabel->setWhatsThis( wtstr );
  urAutostart->setWhatsThis( wtstr );

  row++;
  tmpLabel = new QLabel(i18n("D&ocuments path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urDocument = new KURLRequester(this);
  urDocument->setMode( KFile::Directory );
  tmpLabel->setBuddy( urDocument );
  lay->addMultiCellWidget(urDocument, row, row, 1, RO_LASTCOL);
  connect(urDocument, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This folder will be used by default to "
               "load or save documents from or to.");
  tmpLabel->setWhatsThis( wtstr );
  urDocument->setWhatsThis( wtstr );

  // -- Bottom --
  Q_ASSERT( row == RO_LASTROW-1 ); // if it fails here, check the row++ and RO_LASTROW above

  load();
}

void DesktopPathConfig::load()
{
    // Desktop Paths
    urDesktop->setURL( KGlobalSettings::desktopPath() );
    urAutostart->setURL( KGlobalSettings::autostartPath() );
    urDocument->setURL( KGlobalSettings::documentPath() );
    changed();
}

void DesktopPathConfig::defaults()
{
    // Desktop Paths - keep defaults in sync with kglobalsettings.cpp
    urDesktop->setURL( QDir::homeDirPath() + "/Desktop/" );
    urAutostart->setURL( KGlobal::dirs()->localkdedir() + "Autostart/" );
    urDocument->setURL( QDir::homeDirPath() );
}

void DesktopPathConfig::save()
{
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cgs( config, "Paths" );

    bool pathChanged = false;
    bool autostartMoved = false;

    KURL desktopURL;
    desktopURL.setPath( KGlobalSettings::desktopPath() );
    KURL newDesktopURL;
    newDesktopURL.setPath(urDesktop->url());

    KURL autostartURL;
    autostartURL.setPath( KGlobalSettings::autostartPath() );
    KURL newAutostartURL;
    newAutostartURL.setPath(urAutostart->url());

    KURL documentURL;
    documentURL.setPath( KGlobalSettings::documentPath() );
    KURL newDocumentURL;
    newDocumentURL.setPath(urDocument->url());

    if ( !newDesktopURL.equals( desktopURL, true ) )
    {
        // Test which other paths were inside this one (as it is by default)
        // and for each, test where it should go.
        // * Inside destination -> let them be moved with the desktop (but adjust name if necessary)
        // * Not inside destination -> move first
        // !!!
        kdDebug() << "desktopURL=" << desktopURL.url() << endl;
        QString urlDesktop = urDesktop->url();
        if ( !urlDesktop.endsWith( "/" ))
            urlDesktop+="/";

        if ( desktopURL.isParentOf( autostartURL ) )
        {
            kdDebug() << "Autostart is on the desktop" << endl;

            // Either the Autostart field wasn't changed (-> need to update it)
            if ( newAutostartURL.equals( autostartURL, true ) )
            {
                // Hack. It could be in a subdir inside desktop. Hmmm... Argl.
                urAutostart->setURL( urlDesktop + "Autostart/" );
                kdDebug() << "Autostart is moved with the desktop" << endl;
                autostartMoved = true;
            }
            // or it has been changed (->need to move it from here)
            else
            {
                KURL futureAutostartURL;
                futureAutostartURL.setPath( urlDesktop + "Autostart/" );
                if ( newAutostartURL.equals( futureAutostartURL, true ) )
                    autostartMoved = true;
                else
                    autostartMoved = moveDir( KURL( KGlobalSettings::autostartPath() ), KURL( urAutostart->url() ), i18n("Autostart") );
            }
        }

        if ( moveDir( KURL( KGlobalSettings::desktopPath() ), KURL( urlDesktop ), i18n("Desktop") ) )
        {
//            config->writeEntry( "Desktop", urDesktop->url());
            config->writePathEntry( "Desktop", urlDesktop, true, true );
            pathChanged = true;
        }
    }

    if ( !newAutostartURL.equals( autostartURL, true ) )
    {
        if (!autostartMoved)
            autostartMoved = moveDir( KURL( KGlobalSettings::autostartPath() ), KURL( urAutostart->url() ), i18n("Autostart") );
        if (autostartMoved)
        {
//            config->writeEntry( "Autostart", Autostart->url());
            config->writePathEntry( "Autostart", urAutostart->url(), true, true );
            pathChanged = true;
        }
    }

    if ( !newDocumentURL.equals( documentURL, true ) )
    {
        bool pathOk = true;
        QString path = urDocument->url();
        if (!QDir(path).exists())
        {
            if (!KStandardDirs::makeDir(path))
            {
                KMessageBox::sorry(this, KIO::buildErrorString(KIO::ERR_COULD_NOT_MKDIR, path));
                urDocument->setURL(documentURL.path());
                pathOk = false;
            }
        }

        if (pathOk)
        {
            config->writePathEntry( "Documents", path, true, true );
            pathChanged = true;
        }
    }

    config->sync();

    if (pathChanged)
    {
        kdDebug() << "DesktopPathConfig::save sending message SettingsChanged" << endl;
        KIPC::sendMessageAll(KIPC::SettingsChanged, KApplication::SETTINGS_PATHS);
    }

    // Tell kdesktop about the new config file
    if ( !kapp->dcopClient()->isAttached() )
       kapp->dcopClient()->attach();
    QByteArray data;

    int konq_screen_number = KApplication::desktop()->primaryScreen();
    Q3CString appname;
    if (konq_screen_number == 0)
        appname = "kdesktop";
    else
        appname.sprintf("kdesktop-screen-%d", konq_screen_number);
    kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );
}

bool DesktopPathConfig::moveDir( const KURL & src, const KURL & dest, const QString & type )
{
    if (!src.isLocalFile() || !dest.isLocalFile())
        return true;
    m_ok = true;
    // Ask for confirmation before moving the files
    if ( KMessageBox::questionYesNo( this, i18n("The path for '%1' has been changed;\ndo you want the files to be moved from '%2' to '%3'?").
                              arg(type).arg(src.path()).arg(dest.path()), i18n("Confirmation Required"),i18n("Move"),KStdGuiItem::cancel() )
            == KMessageBox::Yes )
    {
        bool destExists = QFile::exists(dest.path());
        if (destExists)
        {
            m_copyToDest = dest;
            m_copyFromSrc = src;
            KIO::ListJob* job = KIO::listDir( src );
            connect( job, SIGNAL( entries( KIO::Job *, const KIO::UDSEntryList& ) ),
                     this, SLOT( slotEntries( KIO::Job *, const KIO::UDSEntryList& ) ) );
            qApp->enter_loop();

            if (m_ok)
            {
                KIO::del( src );
            }
        }
        else
        {
            KIO::Job * job = KIO::move( src, dest );
            connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
            // wait for job
            qApp->enter_loop();
        }
    }
    kdDebug() << "DesktopPathConfig::slotResult returning " << m_ok << endl;
    return m_ok;
}

void DesktopPathConfig::slotEntries( KIO::Job * job, const KIO::UDSEntryList& list)
{
    if (job->error())
    {
        job->showErrorDialog(this);
        return;
    }

    KIO::UDSEntryListConstIterator it = list.begin();
    KIO::UDSEntryListConstIterator end = list.end();
    for (; it != end; ++it)
    {
        KFileItem file(*it, m_copyFromSrc, true, true);
        if (file.url() == m_copyFromSrc || file.url().fileName() == "..")
        {
            continue;
        }

        KIO::Job * moveJob = KIO::move( file.url(), m_copyToDest );
        connect( moveJob, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotResult( KIO::Job * ) ) );
        qApp->enter_loop();
    }
    qApp->exit_loop();
}

void DesktopPathConfig::slotResult( KIO::Job * job )
{
    if (job->error())
    {
        if ( job->error() != KIO::ERR_DOES_NOT_EXIST )
            m_ok = false;
        // If the source doesn't exist, no wonder we couldn't move the dir.
        // In that case, trust the user and set the new setting in any case.

        job->showErrorDialog(this);
    }
    qApp->exit_loop();
}

#include "rootopts.moc"
