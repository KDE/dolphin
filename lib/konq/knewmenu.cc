/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
                 2003       Sven Leiber <s.leiber@web.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QDir>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QList>
#include <kactioncollection.h>
#include <kseparatoraction.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirwatch.h>
#include <kicon.h>
#include <kinstance.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <kprotocolmanager.h>
#include <kmenu.h>
#include <krun.h>
#include <kactioncollection.h>
#include <kseparatoraction.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/renamedlg.h>

#include <kpropertiesdialog.h>
#include "konq_operations.h"
#include "konq_undo.h"
#include "knewmenu.h"
#include <utime.h>

// For KUrlDesktopFileDlg
#include <QLayout>
#include <klineedit.h>
#include <kurlrequester.h>
#include <QLabel>

QList<KNewMenu::Entry> * KNewMenu::s_templatesList = 0L;
int KNewMenu::s_templatesVersion = 0;
bool KNewMenu::s_filesParsed = false;
KDirWatch * KNewMenu::s_pDirWatch = 0L;

class KNewMenu::KNewMenuPrivate
{
public:
    KNewMenuPrivate() : m_parentWidget(0) {}
    KActionCollection * m_actionCollection;
    QString m_destPath;
    QWidget *m_parentWidget;
    KActionMenu *m_menuDev;
};

KNewMenu::KNewMenu( KActionCollection * _collec, const char *name ) :
  KActionMenu( KIcon("filenew"), i18n( "Create New" ), _collec, name ),
  menuItemsVersion( 0 ),
  m_newMenuGroup(new QActionGroup(this))
{
    //kDebug(1203) << "KNewMenu::KNewMenu " << this << endl;
    // Don't fill the menu yet
    // We'll do that in slotCheckUpToDate (should be connected to abouttoshow)
    d = new KNewMenuPrivate;
    d->m_actionCollection = _collec;
    makeMenus();
}

KNewMenu::KNewMenu( KActionCollection * _collec, QWidget *parentWidget, const char *name ) :
  KActionMenu( KIcon("filenew"), i18n( "Create New" ), _collec, name ),
  menuItemsVersion( 0 ),
  m_newMenuGroup(new QActionGroup(this))
{
    d = new KNewMenuPrivate;
    d->m_actionCollection = _collec;
    d->m_parentWidget = parentWidget;
    makeMenus();
}

KNewMenu::~KNewMenu()
{
    //kDebug(1203) << "KNewMenu::~KNewMenu " << this << endl;
    delete d;
}

void KNewMenu::makeMenus()
{
    d->m_menuDev = new KActionMenu( KIcon("kcmdevices"), i18n( "Link to Device" ), d->m_actionCollection, "devnew" );
}

void KNewMenu::slotCheckUpToDate( )
{
    //kDebug(1203) << "KNewMenu::slotCheckUpToDate() " << this
    //              << " : menuItemsVersion=" << menuItemsVersion
    //              << " s_templatesVersion=" << s_templatesVersion << endl;
    if (menuItemsVersion < s_templatesVersion || s_templatesVersion == 0)
    {
        //kDebug(1203) << "KNewMenu::slotCheckUpToDate() : recreating actions" << endl;
        // We need to clean up the action collection
        // We look for our actions using the group
        foreach (QAction* action, m_newMenuGroup->actions())
            delete action;

        if (!s_templatesList) { // No templates list up to now
            s_templatesList = new QList<Entry>();
            slotFillTemplates();
            parseFiles();
        }

        // This might have been already done for other popupmenus,
        // that's the point in s_filesParsed.
        if ( !s_filesParsed )
            parseFiles();

        fillMenu();

        menuItemsVersion = s_templatesVersion;
    }
}

void KNewMenu::parseFiles()
{
    //kDebug(1203) << "KNewMenu::parseFiles()" << endl;
    s_filesParsed = true;
    QList<Entry>::Iterator templ = s_templatesList->begin();
    for ( /*++templ*/; templ != s_templatesList->end(); ++templ)
    {
        QString iconname;
        QString filePath = (*templ).filePath;
        if ( !filePath.isEmpty() )
        {
            QString text;
            QString templatePath;
            // If a desktop file, then read the name from it.
            // Otherwise (or if no name in it?) use file name
            if ( KDesktopFile::isDesktopFile( filePath ) ) {
                KSimpleConfig config( filePath, true );
                config.setDesktopGroup();
                text = config.readEntry("Name");
                (*templ).icon = config.readEntry("Icon");
                (*templ).comment = config.readEntry("Comment");
                QString type = config.readEntry( "Type" );
                if ( type == "Link" )
                {
                    templatePath = config.readPathEntry("URL");
                    if ( templatePath[0] != '/' )
                    {
                        if ( templatePath.startsWith("file:/") )
                            templatePath = KUrl(templatePath).path();
                        else
                        {
                            // A relative path, then (that's the default in the files we ship)
                            QString linkDir = filePath.left( filePath.lastIndexOf( '/' ) + 1 /*keep / */ );
                            //kDebug(1203) << "linkDir=" << linkDir << endl;
                            templatePath = linkDir + templatePath;
                        }
                    }
                }
                if ( templatePath.isEmpty() )
                {
                    // No dest, this is an old-style template
                    (*templ).entryType = TEMPLATE;
                    (*templ).templatePath = (*templ).filePath; // we'll copy the file
                } else {
                    (*templ).entryType = LINKTOTEMPLATE;
                    (*templ).templatePath = templatePath;
                }

            }
            if (text.isEmpty())
            {
                text = KUrl(filePath).fileName();
                if ( text.endsWith(".desktop") )
                    text.truncate( text.length() - 8 );
                else if ( text.endsWith(".kdelnk") )
                    text.truncate( text.length() - 7 );
            }
            (*templ).text = text;
            /*kDebug(1203) << "Updating entry with text=" << text
                          << " entryType=" << (*templ).entryType
                          << " templatePath=" << (*templ).templatePath << endl;*/
        }
        else {
            (*templ).entryType = SEPARATOR;
        }
    }
}

void KNewMenu::fillMenu()
{
    //kDebug(1203) << "KNewMenu::fillMenu()" << endl;
    menu()->clear();
    d->m_menuDev->menu()->clear();

    KAction *linkURL = 0, *linkApp = 0;  // these shall be put at special positions

    int i = 1; // was 2 when there was Folder
    QList<Entry>::Iterator templ = s_templatesList->begin();
    for ( ; templ != s_templatesList->end(); ++templ, ++i)
    {
        if ( (*templ).entryType != SEPARATOR )
        {
            // There might be a .desktop for that one already, if it's a kdelnk
            // This assumes we read .desktop files before .kdelnk files ...

            // In fact, we skip any second item that has the same text as another one.
            // Duplicates in a menu look bad in any case.

            bool bSkip = false;

            QList<KAction*> actions = d->m_actionCollection->actions();
            QList<KAction*>::Iterator it = actions.begin();
            for( ; it != actions.end() && !bSkip; ++it )
            {
                if ( (*it)->text() == (*templ).text )
                {
                    kDebug(1203) << "KNewMenu: skipping " << (*templ).filePath << endl;
                    bSkip = true;
                }
            }

            if ( !bSkip )
            {
                Entry entry = (s_templatesList->at( i-1 ));

                // The best way to identify the "Create Directory", "Link to Location", "Link to Application" was the template
                if ( (*templ).templatePath.endsWith( "emptydir" ) )
                {
                    KAction * act = new KAction( KIcon((*templ).icon), (*templ).text, d->m_actionCollection, QString("newmenu%1").arg( i ).toUtf8() );
                    connect(act, SIGNAL(triggered()), this, SLOT(slotNewDir()));
                    act->setActionGroup( m_newMenuGroup );
                    menu()->addAction( act );

                    KSeparatorAction *sep = new KSeparatorAction();
                    menu()->addAction( sep );
                }
                else
                {
                    KAction * act = new KAction( KIcon((*templ).icon), (*templ).text, d->m_actionCollection, QString("newmenu%1").arg( i ).toUtf8() );
                    connect(act, SIGNAL(triggered()), this, SLOT(slotNewFile()));
                    act->setActionGroup( m_newMenuGroup );

                    if ( (*templ).templatePath.endsWith( "URL.desktop" ) )
                    {
                        linkURL = act;
                    }
                    else if ( (*templ).templatePath.endsWith( "Program.desktop" ) )
                    {
                        linkApp = act;
                    }
                    else if ( KDesktopFile::isDesktopFile( entry.templatePath ) )
                    {
                        KDesktopFile df( entry.templatePath );
                        if(df.readType() == "FSDevice")
                            d->m_menuDev->menu()->addAction( act );
                        else
                            menu()->addAction( act );
                    }
                    else
                    {
                        menu()->addAction( act );
                    }
                }
            }
        } else { // Separate system from personal templates
            Q_ASSERT( (*templ).entryType != 0 );

            KSeparatorAction * act = new KSeparatorAction();
            menu()->addAction( act );
        }
    }

    KSeparatorAction * act = new KSeparatorAction();
    menu()->addAction( act );
    if ( linkURL ) menu()->addAction( linkURL );
    if ( linkApp ) menu()->addAction( linkApp );
    menu()->addAction( d->m_menuDev );
}

void KNewMenu::slotFillTemplates()
{
    //kDebug(1203) << "KNewMenu::slotFillTemplates()" << endl;
    // Ensure any changes in the templates dir will call this
    if ( ! s_pDirWatch )
    {
        s_pDirWatch = new KDirWatch;
        QStringList dirs = d->m_actionCollection->instance()->dirs()->resourceDirs("templates");
        for ( QStringList::Iterator it = dirs.begin() ; it != dirs.end() ; ++it )
        {
            //kDebug(1203) << "Templates resource dir: " << *it << endl;
            s_pDirWatch->addDir( *it );
        }
        connect ( s_pDirWatch, SIGNAL( dirty( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        connect ( s_pDirWatch, SIGNAL( created( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        connect ( s_pDirWatch, SIGNAL( deleted( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        // Ok, this doesn't cope with new dirs in KDEDIRS, but that's another story
    }
    s_templatesVersion++;
    s_filesParsed = false;

    s_templatesList->clear();

    // Look into "templates" dirs.
    QStringList files = d->m_actionCollection->instance()->dirs()->findAllResources("templates");
    QMap<QString, Entry> slist; // used for sorting
    for ( QStringList::Iterator it = files.begin() ; it != files.end() ; ++it )
    {
        //kDebug(1203) << *it << endl;
        if ( (*it)[0] != '.' )
        {
            Entry e;
            e.filePath = *it;
            e.entryType = 0; // not parsed yet
            // put Directory etc. with special order (see fillMenu()) first in the list (a bit hacky)
            if ( (*it).endsWith( "Directory.desktop" ) ||
                 (*it).endsWith( "linkProgram.desktop" ) ||
                 (*it).endsWith( "linkURL.desktop" ) )
                s_templatesList->prepend( e );
            else
            {
                KSimpleConfig config( *it, true );
                config.setDesktopGroup();

                // tricky solution to ensure that TextFile is at the beginning
                // because this filetype is the most used (according kde-core discussion)
                QString key = config.readEntry("Name");
                if ( (*it).endsWith( "TextFile.desktop" ) )
                    key.prepend( '1' );
                else
                    key.prepend( '2' );

                slist.insert( key, e );
            }
        }
    }
    for(QMap<QString, Entry>::const_iterator it = slist.begin(); it != slist.end(); ++it)
    {
        s_templatesList->append( it.value() );
    }

}

void KNewMenu::slotNewDir()
{
    slotTriggered(); // for KDIconView::slotNewMenuActivated()

    if (popupFiles.isEmpty())
       return;

    KonqOperations::newDir(d->m_parentWidget, popupFiles.first());
}

void KNewMenu::slotNewFile()
{
    int id = QString( sender()->objectName().mid( 7 ) ).toInt(); // skip "newmenu"
    if (id == 0)
    {
	// run the command for the templates
	KRun::runCommand(QString(sender()->objectName()));
	return;
    }

    slotTriggered(); // for KDIconView::slotNewMenuActivated()

    Entry entry = (s_templatesList->at( id - 1 ));
    //kDebug(1203) << QString("sFile = %1").arg(sFile) << endl;

    if ( !QFile::exists( entry.templatePath ) ) {
        kWarning(1203) << entry.templatePath << " doesn't exist" << endl;
        KMessageBox::sorry( 0L, i18n("<qt>The template file <b>%1</b> does not exist.</qt>", entry.templatePath));
        return;
    }
    m_isURLDesktopFile = false;
    QString name;
    if ( KDesktopFile::isDesktopFile( entry.templatePath ) )
    {
	KDesktopFile df( entry.templatePath );
    	//kDebug(1203) <<  df.readType() << endl;
    	if ( df.readType() == "Link" )
    	{
    	    m_isURLDesktopFile = true;
    	    // entry.comment contains i18n("Enter link to location (URL):"). JFYI :)
    	    KUrlDesktopFileDlg dlg( i18n("File name:"), entry.comment, d->m_parentWidget );
    	    // TODO dlg.setCaption( i18n( ... ) );
    	    if ( dlg.exec() )
    	    {
                name = dlg.fileName();
                m_linkURL = dlg.url();
                if ( name.isEmpty() || m_linkURL.isEmpty() )
        	    return;
            	if ( !name.endsWith( ".desktop" ) )
            	    name += ".desktop";
    	    }
    	    else
                return;
    	}
    	else // any other desktop file (Device, App, etc.)
    	{
    	    KUrl::List::Iterator it = popupFiles.begin();
    	    for ( ; it != popupFiles.end(); ++it )
    	    {
                //kDebug(1203) << "first arg=" << entry.templatePath << endl;
                //kDebug(1203) << "second arg=" << (*it).url() << endl;
                //kDebug(1203) << "third arg=" << entry.text << endl;
                QString text = entry.text;
                text.replace( "...", QString() ); // the ... is fine for the menu item but not for the default filename

		KUrl defaultFile( *it );
		defaultFile.addPath( KIO::encodeFileName( text ) );
		if ( defaultFile.isLocalFile() && QFile::exists( defaultFile.path() ) )
		    text = KIO::RenameDlg::suggestName( *it, text);

                KUrl templateURL;
                templateURL.setPath( entry.templatePath );
                (void) new KPropertiesDialog( templateURL, *it, text, d->m_parentWidget );
    	    }
    	    return; // done, exit.
    	}
    }
    else
    {
        // The template is not a desktop file
        // Show the small dialog for getting the destination filename
        bool ok;
        QString text = entry.text;
        text.replace( "...", QString() ); // the ... is fine for the menu item but not for the default filename

	KUrl defaultFile( *(popupFiles.begin()) );
	defaultFile.addPath( KIO::encodeFileName( text ) );
	if ( defaultFile.isLocalFile() && QFile::exists( defaultFile.path() ) )
	    text = KIO::RenameDlg::suggestName( *(popupFiles.begin()), text);

        name = KInputDialog::getText( QString(), entry.comment,
    	text, &ok, d->m_parentWidget );
        if ( !ok )
	    return;
    }

    // The template is not a desktop file [or it's a URL one]
    // Copy it.
    KUrl::List::Iterator it = popupFiles.begin();

    QString src = entry.templatePath;
    for ( ; it != popupFiles.end(); ++it )
    {
        KUrl dest( *it );
        dest.addPath( KIO::encodeFileName(name) ); // Chosen destination file name
        d->m_destPath = dest.path(); // will only be used if m_isURLDesktopFile and dest is local

        KUrl uSrc;
        uSrc.setPath( src );
        //kDebug(1203) << "KNewMenu : KIO::copyAs( " << uSrc.url() << ", " << dest.url() << ")" << endl;
        KIO::CopyJob * job = KIO::copyAs( uSrc, dest );
        job->setDefaultPermissions( true );
        connect( job, SIGNAL( result( KJob * ) ),
                SLOT( slotResult( KJob * ) ) );
        if ( m_isURLDesktopFile )
		connect( job, SIGNAL( renamed( KIO::Job *, const KUrl&, const KUrl& ) ),
        	     SLOT( slotRenamed( KIO::Job *, const KUrl&, const KUrl& ) ) );
    	KUrl::List lst;
    	lst.append( uSrc );
    	(void)new KonqCommandRecorder( KonqCommand::COPY, lst, dest, job );
    }
}

// Special case (filename conflict when creating a link=url file)
// We need to update m_destURL
void KNewMenu::slotRenamed( KIO::Job *, const KUrl& from , const KUrl& to )
{
    if ( from.isLocalFile() )
    {
        kDebug() << k_funcinfo << from.prettyUrl() << " -> " << to.prettyUrl() << " ( m_destPath=" << d->m_destPath << ")" << endl;
        Q_ASSERT( from.path() == d->m_destPath );
        d->m_destPath = to.path();
    }
}

void KNewMenu::slotResult( KJob * job )
{
    if (job->error())
    {
        static_cast<KIO::Job*>( job )->ui()->setWindow( 0 );
        static_cast<KIO::Job*>( job )->ui()->showErrorMessage();
    }
    else
    {
        KUrl destURL = static_cast<KIO::CopyJob*>(job)->destURL();
        if ( destURL.isLocalFile() )
        {
            if ( m_isURLDesktopFile )
            {
                // destURL is the original destination for the new file.
                // But in case of a renaming (due to a conflict), the real path is in m_destPath
                kDebug(1203) << " destURL=" << destURL.path() << " " << " d->m_destPath=" << d->m_destPath << endl;
                KDesktopFile df( d->m_destPath );
                df.writeEntry( "Icon", KProtocolInfo::icon( m_linkURL.protocol() ) );
                df.writePathEntry( "URL", m_linkURL.prettyUrl() );
                df.sync();
            }
            else
            {
                // Normal (local) file. Need to "touch" it, kio_file copied the mtime.
                (void) ::utime( QFile::encodeName( destURL.path() ), 0 );
            }
        }
    }
}

//////////

KUrlDesktopFileDlg::KUrlDesktopFileDlg( const QString& textFileName, const QString& textUrl, QWidget *parent )
    : KDialog( parent )
{
    setModal( true );
    setButtons( Ok | Cancel | User1 );
    setButtonGuiItem( User1, KStdGuiItem::clear() );
    showButtonSeparator( true );

    initDialog( textFileName, QString(), textUrl, QString() );
}

void KUrlDesktopFileDlg::initDialog( const QString& textFileName, const QString& defaultName, const QString& textUrl, const QString& defaultUrl )
{
    QFrame *plainPage = new QFrame( this );
    setMainWidget( plainPage );

    QVBoxLayout * topLayout = new QVBoxLayout( plainPage );
    topLayout->setMargin( 0 );
    topLayout->setSpacing( spacingHint() );

    // First line: filename
    KHBox * fileNameBox = new KHBox( plainPage );
    topLayout->addWidget( fileNameBox );

    QLabel * label = new QLabel( textFileName, fileNameBox );
    m_leFileName = new KLineEdit( fileNameBox );
    m_leFileName->setMinimumWidth(m_leFileName->sizeHint().width() * 3);
    label->setBuddy(m_leFileName);  // please "scheck" style
    m_leFileName->setText( defaultName );
    m_leFileName->setSelection(0, m_leFileName->text().length()); // autoselect
    connect( m_leFileName, SIGNAL(textChanged(const QString&)),
             SLOT(slotNameTextChanged(const QString&)) );

    // Second line: url
    KHBox * urlBox = new KHBox( plainPage );
    topLayout->addWidget( urlBox );
    label = new QLabel( textUrl, urlBox );
    m_urlRequester = new KUrlRequester( defaultUrl, urlBox);
	m_urlRequester->setObjectName("urlRequester");
    m_urlRequester->setMode( KFile::File | KFile::Directory );

    m_urlRequester->setMinimumWidth( m_urlRequester->sizeHint().width() * 3 );
    connect( m_urlRequester->lineEdit(), SIGNAL(textChanged(const QString&)),
             SLOT(slotURLTextChanged(const QString&)) );
    label->setBuddy(m_urlRequester);  // please "scheck" style

    m_urlRequester->setFocus();
    enableButtonOk( !defaultName.isEmpty() && !defaultUrl.isEmpty() );
    connect( this, SIGNAL(user1Clicked()), this, SLOT(slotClear()) );
    m_fileNameEdited = false;
}

KUrl KUrlDesktopFileDlg::url() const
{
    if ( result() == QDialog::Accepted )
        return m_urlRequester->url();
    else
        return KUrl();
}

QString KUrlDesktopFileDlg::fileName() const
{
    if ( result() == QDialog::Accepted )
        return m_leFileName->text();
    else
        return QString();
}

void KUrlDesktopFileDlg::slotClear()
{
    m_leFileName->setText( QString() );
    m_urlRequester->clear();
    m_fileNameEdited = false;
}

void KUrlDesktopFileDlg::slotNameTextChanged( const QString& )
{
    kDebug() << k_funcinfo << endl;
    m_fileNameEdited = true;
    enableButtonOk( !m_leFileName->text().isEmpty() && !m_urlRequester->url().isEmpty() );
}

void KUrlDesktopFileDlg::slotURLTextChanged( const QString& )
{
    if ( !m_fileNameEdited )
    {
        // use URL as default value for the filename
        // (we copy only its filename if protocol supports listing,
        // but for HTTP we don't want tons of index.html links)
        KUrl url( m_urlRequester->url() );
        if ( KProtocolManager::supportsListing( url ) )
            m_leFileName->setText( url.fileName() );
        else
            m_leFileName->setText( url.url() );
        m_fileNameEdited = false; // slotNameTextChanged set it to true erroneously
    }
    enableButtonOk( !m_leFileName->text().isEmpty() && !m_urlRequester->url().isEmpty() );
}


#include "knewmenu.moc"
