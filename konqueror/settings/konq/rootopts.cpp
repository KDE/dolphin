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
#include <qvgroupbox.h>
#include <qwhatsthis.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kglobalsettings.h>
#include <klistview.h>
#include <klocale.h>
#include <kipc.h>
#include <kmessagebox.h>
#include <ktrader.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kmimetype.h>
#include <kcustommenueditor.h>

#include <dcopclient.h>
#include <kio/job.h>

#include "rootopts.h"

#include <konq_defaults.h> // include default values directly from libkonq


//-----------------------------------------------------------------------------

DesktopPathConfig::DesktopPathConfig(QWidget *parent, const char * )
    : KCModule( parent, "kcmkonq" )
{
  QLabel * tmpLabel;

#undef RO_LASTROW
#undef RO_LASTCOL
#define RO_LASTROW 5   // 4 paths + last row
#define RO_LASTCOL 2

  int row = 0;
  QGridLayout *lay = new QGridLayout(this, RO_LASTROW+1, RO_LASTCOL+1,
      0, KDialog::spacingHint());

  lay->setRowStretch(RO_LASTROW,10); // last line grows

  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,10);

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
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( urDesktop, wtstr );

  row++;
  tmpLabel = new QLabel(i18n("&Trash path:"), this);
  lay->addWidget(tmpLabel, row, 0);
  urTrash = new KURLRequester(this);
  urTrash->setMode( KFile::Directory );
  tmpLabel->setBuddy( urTrash );
  lay->addMultiCellWidget(urTrash, row, row, 1, RO_LASTCOL);
  connect(urTrash, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  wtstr = i18n("This folder contains deleted files (until"
               " you empty the trashcan). You can change the location of this"
               " folder if you want to, and the contents will move automatically"
               " to the new location as well.");
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( urTrash, wtstr );

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
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( urAutostart, wtstr );

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
  QWhatsThis::add( tmpLabel, wtstr );
  QWhatsThis::add( urDocument, wtstr );

  // -- Bottom --
  Q_ASSERT( row == RO_LASTROW-1 ); // if it fails here, check the row++ and RO_LASTROW above

  load();
}

void DesktopPathConfig::load()
{
    // Desktop Paths
    urDesktop->setURL( KGlobalSettings::desktopPath() );
    urTrash->setURL( KGlobalSettings::trashPath() );
    urAutostart->setURL( KGlobalSettings::autostartPath() );
    urDocument->setURL( KGlobalSettings::documentPath() );
    changed();
}

void DesktopPathConfig::defaults()
{
    // Desktop Paths - keep defaults in sync with kglobalsettings.cpp
    urDesktop->setURL( QDir::homeDirPath() + "/Desktop/" );
    urTrash->setURL( QDir::homeDirPath() + "/Desktop/" + i18n("Trash") + '/' );
    urAutostart->setURL( KGlobal::dirs()->localkdedir() + "Autostart/" );
    urDocument->setURL( QDir::homeDirPath() );
}

void DesktopPathConfig::save()
{
    KConfig *config = KGlobal::config();
    KConfigGroupSaver cgs( config, "Paths" );

    bool pathChanged = false;
    bool trashMoved = false;
    bool autostartMoved = false;

    KURL desktopURL;
    desktopURL.setPath( KGlobalSettings::desktopPath() );
    KURL newDesktopURL;
    newDesktopURL.setPath(urDesktop->url());

    KURL trashURL;
    trashURL.setPath( KGlobalSettings::trashPath() );
    KURL newTrashURL;
    newTrashURL.setPath(urTrash->url());

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
        kdDebug() << "trashURL=" << trashURL.url() << endl;
        QString urlDesktop = urDesktop->url();
        if ( !urlDesktop.endsWith( "/" ))
            urlDesktop+="/";

        if ( desktopURL.isParentOf( trashURL ) )
        {
            // The trash is on the desktop (no, I don't do this at home....)
            kdDebug() << "The trash is currently on the desktop" << endl;
            // Either the Trash field wasn't changed (-> need to update it)
            if ( newTrashURL.equals( trashURL, true ) )
            {
                // Hack. It could be in a subdir inside desktop. Hmmm... Argl.
                urTrash->setURL( urlDesktop + trashURL.fileName() );
                kdDebug() << "The trash is moved with the desktop" << endl;
                trashMoved = true;
            }
            // or it has been changed (->need to move it from here, unless moving the desktop does it)
            else
            {
                KURL futureTrashURL;
                futureTrashURL.setPath( urlDesktop + trashURL.fileName() );
                if ( newTrashURL.equals( futureTrashURL, true ) )
                    trashMoved = true; // The trash moves with the desktop
                else
                    trashMoved = moveDir( KURL( KGlobalSettings::trashPath() ), KURL( urTrash->url() ), i18n("Trash") );
            }
        }

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

    if ( !newTrashURL.equals( trashURL, true ) )
    {
        if (!trashMoved)
            trashMoved = moveDir( KURL( KGlobalSettings::trashPath() ), KURL( urTrash->url() ), i18n("Trash") );
        if (trashMoved)
        {
//            config->writeEntry( "Trash", urTrash->url());
            config->writePathEntry( "Trash", urTrash->url(), true, true );
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
//        config->writeEntry( "Documents", urDocument->url());
        config->writePathEntry( "Documents", urDocument->url(), true, true );
        pathChanged = true;
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
    QCString appname;
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
    if ( KMessageBox::questionYesNo( this, i18n("The path for '%1' has been changed.\nDo you want me to move the files from '%2' to '%3'?").
                              arg(type).arg(src.path()).arg(dest.path()), i18n("Confirmation required") )
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
        if (file.url() == m_copyFromSrc || file.url().filename() == "..")
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

void DesktopPathConfig::changed()
{
  emit KCModule::changed(true);
}


QString DesktopPathConfig::quickHelp() const
{
  return i18n("<h1>Paths</h1>\n"
    "This module allows you to choose where in the filesystem the "
    "files on your desktop should be stored.\n"
    "Use the \"Whats This?\" (Shift+F1) to get help on specific options.");
}


#include "rootopts.moc"
