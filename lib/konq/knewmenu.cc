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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qdir.h>

#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirwatch.h>
#include <kinstance.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <kpopupmenu.h>
#include <krun.h>

#include <kio/job.h>

#include <kpropertiesdialog.h>
//#include <konq_dirwatcher_stub.h>
#include "konq_undo.h"
#include "knewmenu.h"
#include <utime.h>

// For KURLDesktopFileDlg
#include <qlayout.h>
#include <qhbox.h>
#include <klineedit.h>
#include <kurlrequester.h>
#include <qlabel.h>
#include <qpopupmenu.h>

QValueList<KNewMenu::Entry> * KNewMenu::s_templatesList = 0L;
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
    KActionMenu *m_menuNew;
    KActionMenu *m_template;
    KActionMenu *m_kword;
    KActionMenu *m_kspread;
    KActionMenu *m_kpresenter;
    KActionMenu *m_kivio;
    KActionMenu *m_kugar;
    KActionMenu *m_kugardesigner;
};

KNewMenu::KNewMenu( KActionCollection * _collec, const char *name ) :
  KActionMenu( i18n( "Create New" ), "filenew", _collec, name ),
  menuItemsVersion( 0 )
{
    //kdDebug(1203) << "KNewMenu::KNewMenu " << this << endl;
  // Don't fill the menu yet
  // We'll do that in slotCheckUpToDate (should be connected to abouttoshow)
    d = new KNewMenuPrivate;
    d->m_actionCollection = _collec;
    makeMenus();
}

KNewMenu::KNewMenu( KActionCollection * _collec, QWidget *parentWidget, const char *name ) :
  KActionMenu( i18n( "Create New" ), "filenew", _collec, name ),
  menuItemsVersion( 0 )
{
    d = new KNewMenuPrivate;
    d->m_actionCollection = _collec;
    d->m_parentWidget = parentWidget;
    makeMenus();
}

KNewMenu::~KNewMenu()
{
    //kdDebug(1203) << "KNewMenu::~KNewMenu " << this << endl;
    delete d;
}

void KNewMenu::makeMenus()
{
    d->m_menuDev = new KActionMenu( i18n( "Device" ), "filenew", d->m_actionCollection, "devnew" );
    d->m_menuNew = new KActionMenu( i18n( "File" ), "filenew", d->m_actionCollection, "devnew" );
    d->m_template = new KActionMenu( i18n( "From Template" ), "filenew" );
    d->m_kword = new KActionMenu( i18n( "KWord" ), "kword" );
    d->m_kspread = new KActionMenu( i18n( "KSpread" ), "kspread" );
    d->m_kpresenter = new KActionMenu( i18n( "KPresenter" ), "kpresenter" );
    d->m_kivio = new KActionMenu( i18n( "Kivio" ), "kivio" );
    d->m_kugar = new KActionMenu( i18n( "Kugar" ), "kugar" );
    d->m_kugardesigner = new KActionMenu( i18n( "Kugar Designer" ), "kugar" );
}

void KNewMenu::slotCheckUpToDate( )
{
    //kdDebug(1203) << "KNewMenu::slotCheckUpToDate() " << this
    //              << " : menuItemsVersion=" << menuItemsVersion
    //              << " s_templatesVersion=" << s_templatesVersion << endl;
    if (menuItemsVersion < s_templatesVersion || s_templatesVersion == 0)
    {
        //kdDebug(1203) << "KNewMenu::slotCheckUpToDate() : recreating actions" << endl;
        // We need to clean up the action collection
        // We look for our actions using the group
        QValueList<KAction*> actions = d->m_actionCollection->actions( "KNewMenu" );
        for( QValueListIterator<KAction*> it = actions.begin(); it != actions.end(); ++it )
        {
            remove( *it );
            d->m_actionCollection->remove( *it );
        }

        if (!s_templatesList) { // No templates list up to now
            s_templatesList = new QValueList<Entry>();
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
    //kdDebug(1203) << "KNewMenu::parseFiles()" << endl;
    s_filesParsed = true;
    QValueList<Entry>::Iterator templ = s_templatesList->begin();
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
                        if ( templatePath.left(6) == "file:/" )
                            templatePath = templatePath.right( templatePath.length() - 6 );
                        else
                        {
                            // A relative path, then (that's the default in the files we ship)
                            QString linkDir = filePath.left( filePath.findRev( '/' ) + 1 /*keep / */ );
                            //kdDebug(1203) << "linkDir=" << linkDir << endl;
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
                text = KURL(filePath).fileName();
                if ( text.right(8) == ".desktop" )
                    text.truncate( text.length() - 8 );
                else if ( text.right(7) == ".kdelnk" )
                    text.truncate( text.length() - 7 );
            }
            (*templ).text = text;
            /*kdDebug(1203) << "Updating entry with text=" << text
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
    //kdDebug(1203) << "KNewMenu::fillMenu()" << endl;
    popupMenu()->clear();
    d->m_menuDev->popupMenu()->clear();
    d->m_menuNew->popupMenu()->clear();
    d->m_template->popupMenu()->clear();
    d->m_kword->popupMenu()->clear();
    d->m_kspread->popupMenu()->clear();
    d->m_kpresenter->popupMenu()->clear();
    d->m_kivio->popupMenu()->clear();
    d->m_kugar->popupMenu()->clear();
    d->m_kugardesigner->popupMenu()->clear();

    int i = 1; // was 2 when there was Folder
    QValueList<Entry>::Iterator templ = s_templatesList->begin();
    for ( ; templ != s_templatesList->end(); ++templ, ++i)
    {
        if ( (*templ).entryType != SEPARATOR )
        {
            // There might be a .desktop for that one already, if it's a kdelnk
            // This assumes we read .desktop files before .kdelnk files ...

            // In fact, we skip any second item that has the same text as another one.
            // Duplicates in a menu look bad in any case.

            bool bSkip = false;

            QValueList<KAction*> actions = d->m_actionCollection->actions();
            QValueListIterator<KAction*> it = actions.begin();
            for( ; it != actions.end() && !bSkip; ++it )
            {
                if ( (*it)->text() == (*templ).text )
                {
                    kdDebug(1203) << "KNewMenu: skipping " << (*templ).filePath << endl;
                    bSkip = true;
                }
            }

            if ( !bSkip )
            {
                Entry entry = *(s_templatesList->at( i-1 ));

                    // The best way to identify the "Create Directory" was the template
                if((*templ).templatePath.right( 8 ) == "emptydir")
                {
                    KAction * act = new KAction( (*templ).text, (*templ).icon, 0, this, SLOT( slotNewFile() ),
                                     d->m_actionCollection, QCString().sprintf("newmenu%d", i ) );
                    act->setGroup( "KNewMenu" );
            	    act->plug( popupMenu() );
                }
                else if ( KDesktopFile::isDesktopFile( entry.templatePath ) )
                {
                    KDesktopFile df( entry.templatePath );
                    if(df.readType() == "FSDevice")
                    {
                        KAction * act = new KAction( (*templ).text, (*templ).icon, 0, this, SLOT( slotNewFile() ),
                                                 d->m_actionCollection, QCString().sprintf("newmenu%d", i ) );
                        act->setGroup( "KNewMenu" );
                        act->plug( d->m_menuDev->popupMenu() );
                    }
                    else
                    {
                        KAction * act = new KAction( (*templ).text, (*templ).icon, 0, this, SLOT( slotNewFile() ),
                                                 d->m_actionCollection, QCString().sprintf("newmenu%d", i ) );
                        act->setGroup( "KNewMenu" );
                        act->plug( d->m_menuNew->popupMenu() );
                    }
                }
                else
                {
                    KAction * act = new KAction( (*templ).text, (*templ).icon, 0, this, SLOT( slotNewFile() ),
                                    d->m_actionCollection, QCString().sprintf("newmenu%d", i ) );
                    act->setGroup( "KNewMenu" );
            	    act->plug( d->m_menuNew->popupMenu() );
                }
            }
        } else { // Separate system from personal templates
            Q_ASSERT( (*templ).entryType != 0 );

            KActionSeparator * act = new KActionSeparator();
            act->plug( popupMenu() );
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////////
    /// Adding the from template part!
    bool tmpexist = false;
    if(makeKOffice("kword/templates", "kword --template ", d->m_kword->popupMenu()))
    {
        d->m_kword->plug( d->m_template->popupMenu() );
	tmpexist = true;
    }
    if(makeKOffice("kspread/templates", "kspread --template ", d->m_kspread->popupMenu()))
    {
	d->m_kspread->plug( d->m_template->popupMenu() );
	tmpexist = true;
    }
    if(makeKOffice("kpresenter/templates", "kpresenter --template ", d->m_kpresenter->popupMenu()))
    {
	d->m_kpresenter->plug( d->m_template->popupMenu() );
	tmpexist = true;
    }
    if(makeKOffice("kivio/templates", "kivio --template ", d->m_kivio->popupMenu()))
    {
	d->m_kivio->plug( d->m_template->popupMenu() );
	tmpexist = true;
    }
    // At the moment the program was not updatet to the new template layout, so it display no template!  :(
    // hope this will work later
    if(makeKOffice("kugar/templates", "kugar --template ", d->m_kugar->popupMenu()))
    {
	d->m_kugar->plug( d->m_template->popupMenu() );
	tmpexist = true;
    }
    if(makeKOffice("kudesigner/templates", "kudesigner --template ", d->m_kugardesigner->popupMenu()))
    {
	d->m_kugardesigner->plug( d->m_template->popupMenu() );
	tmpexist = true;
    }

    d->m_menuNew->plug( popupMenu() );
    d->m_menuDev->plug( popupMenu() );
    if(tmpexist)
        d->m_template->plug( popupMenu() );
}

bool KNewMenu::makeKOffice( const QString tmp, const QString exec, QPopupMenu *popup )
{
    bool m_return = false;;

    // We whant to have the templates from the homedirectory, too
    QStringList templates = KGlobal::dirs()->findDirs( "data", tmp );
    for ( QStringList::Iterator it = templates.begin() ; it != templates.end() ; ++it )
    {
	//kdDebug(1203) << "Templates resource dir: " << *it << endl;
	QDir dir( *it );

	// Find all category's of templates
	QStringList dirs = dir.entryList( QDir::Dirs );
	for ( QStringList::Iterator dirsit = dirs.begin() ; dirsit != dirs.end() ; ++dirsit )
	{
	    if(*dirsit == "." || *dirsit == "..")
		continue;

	    //kdDebug(1203) << "Template dirs: " << *it+*dirsit << endl;
	    QDir file( *it+*dirsit );
	    // Read all templates
	    QString group = *dirsit;
	    if( file.exists( ".directory" ) )
	    {
		KSimpleConfig config(file.absPath()+"/.directory" );
		config.setDesktopGroup();
		group = config.readEntry( "Name" );
	    }
	    QStringList files = file.entryList( QDir::Files | QDir::Readable, QDir::Name );
	    for ( QStringList::Iterator filesit = files.begin() ; filesit != files.end() ; ++filesit )
	    {
		if( KDesktopFile::isDesktopFile(file.absPath()+"/"+*filesit) )
		{
    		    KSimpleConfig config(file.absPath()+"/"+*filesit);
		    config.setDesktopGroup();
		    if( config.readEntry( "Type" ) == "Link" )
		    {
			QString text = config.readEntry( "Name" );
			QString icon = file.absPath()+"/"+config.readEntry( "Icon" );
			//kdDebug(1203) << "Template Files: " << text << " | " << icon << " | " << file.absPath()+"/"+*filesit << endl;
            		KAction * act = new KAction( group + " - " + text, icon, 0, this, SLOT( slotNewFile() ),
                    			    d->m_actionCollection, QString(exec+*dirsit+"/"+*filesit).ascii() );
            		act->setGroup( "KNewMenu" );
    			act->plug( popup );
			m_return = true;
		    }
		}
	    }
	}
    }

    return m_return;
}

void KNewMenu::slotFillTemplates()
{
    //kdDebug(1203) << "KNewMenu::slotFillTemplates()" << endl;
    // Ensure any changes in the templates dir will call this
    if ( ! s_pDirWatch )
    {
        s_pDirWatch = new KDirWatch;
        QStringList dirs = d->m_actionCollection->instance()->dirs()->resourceDirs("templates");
        for ( QStringList::Iterator it = dirs.begin() ; it != dirs.end() ; ++it )
        {
            //kdDebug(1203) << "Templates resource dir: " << *it << endl;
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
    for ( QStringList::Iterator it = files.begin() ; it != files.end() ; ++it )
    {
        //kdDebug(1203) << *it << endl;
        if ( (*it)[0] != '.' )
        {
            Entry e;
            e.filePath = *it;
            e.entryType = 0; // not parsed yet
            // put Directory first in the list (a bit hacky)
            if ( (*it).endsWith( "Directory.desktop" ) )
                s_templatesList->prepend( e );
            else
                s_templatesList->append( e );
        }
    }
}

void KNewMenu::slotNewFile()
{
    int id = QString( sender()->name() + 7 ).toInt(); // skip "newmenu"
    if (id == 0)
    {
	// run the command for the templates
	KRun::runCommand(QString(sender()->name()));
	return;
    }

    emit activated(); // for KDIconView::slotNewMenuActivated()

    Entry entry = *(s_templatesList->at( id - 1 ));
    //kdDebug(1203) << QString("sFile = %1").arg(sFile) << endl;

    if ( !QFile::exists( entry.templatePath ) ) {
        kdWarning(1203) << entry.templatePath << " doesn't exist" << endl;
        KMessageBox::sorry( 0L, i18n("<qt>The template file <b>%1</b> doesn't exist.</qt>").arg(entry.templatePath));
        return;
    }
    m_isURLDesktopFile = false;
    QString name;
    if ( KDesktopFile::isDesktopFile( entry.templatePath ) )
    {
	KDesktopFile df( entry.templatePath );
    	//kdDebug(1203) <<  df.readType() << endl;
    	if ( df.readType() == "Link" )
    	{
    	    m_isURLDesktopFile = true;
    	    // entry.comment contains i18n("Enter link to location (URL):"). JFYI :)
    	    KURLDesktopFileDlg dlg( i18n("File name:"), entry.comment, d->m_parentWidget );
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
    	else
    	{
    	    KURL::List::Iterator it = popupFiles.begin();
    	    for ( ; it != popupFiles.end(); ++it )
    	    {
                //kdDebug(1203) << "first arg=" << entry.templatePath << endl;
                //kdDebug(1203) << "second arg=" << (*it).url() << endl;
                //kdDebug(1203) << "third arg=" << entry.text << endl;
                (void) new KPropertiesDialog( entry.templatePath, *it, entry.text, d->m_parentWidget );
    	    }
    	    return; // done, exit.
    	}
    }
    else
    {
        // The template is not a desktop file
        // Show the small dialog for getting the destination filename
        bool ok;
        name = KInputDialog::getText( QString::null, entry.comment,
    	entry.text, &ok, d->m_parentWidget );
        if ( !ok )
	    return;
    }

    // The template is not a desktop file [or it's a URL one]
    // Copy it.
    KURL::List::Iterator it = popupFiles.begin();

    QString src = entry.templatePath;
    for ( ; it != popupFiles.end(); ++it )
    {
        KURL dest( *it );
        dest.addPath( KIO::encodeFileName(name) ); // Chosen destination file name
        d->m_destPath = dest.path(); // will only be used if m_isURLDesktopFile and dest is local

        KURL uSrc;
        uSrc.setPath( src );
        //kdDebug(1203) << "KNewMenu : KIO::copyAs( " << uSrc.url() << ", " << dest.url() << ")" << endl;
        KIO::Job * job = KIO::copyAs( uSrc, dest );
        connect( job, SIGNAL( result( KIO::Job * ) ),
                SLOT( slotResult( KIO::Job * ) ) );
        if ( m_isURLDesktopFile )
		connect( job, SIGNAL( renamed( KIO::Job *, const KURL&, const KURL& ) ),
        	     SLOT( slotRenamed( KIO::Job *, const KURL&, const KURL& ) ) );
    	KURL::List lst;
    	lst.append( uSrc );
    	(void)new KonqCommandRecorder( KonqCommand::COPY, lst, dest, job );
    }
}

// Special case (filename conflict when creating a link=url file)
// We need to update m_destURL
void KNewMenu::slotRenamed( KIO::Job *, const KURL& from , const KURL& to )
{
    if ( from.isLocalFile() )
    {
        kdDebug() << k_funcinfo << from.prettyURL() << " -> " << to.prettyURL() << " ( m_destPath=" << d->m_destPath << ")" << endl;
        Q_ASSERT( from.path() == d->m_destPath );
        d->m_destPath = to.path();
    }
}

void KNewMenu::slotResult( KIO::Job * job )
{
    if (job->error())
        job->showErrorDialog();
    else
    {
        KURL destURL = static_cast<KIO::CopyJob*>(job)->destURL();
        if ( destURL.isLocalFile() )
        {
            if ( m_isURLDesktopFile )
            {
                // destURL is the original destination for the new file.
                // But in case of a renaming (due to a conflict), the real path is in m_destPath
                kdDebug(1203) << " destURL=" << destURL.path() << " " << " d->m_destPath=" << d->m_destPath << endl;
                KDesktopFile df( d->m_destPath );
                df.writeEntry( "Icon", KProtocolInfo::icon( KURL(m_linkURL).protocol() ) );
                df.writePathEntry( "URL", m_linkURL );
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

KURLDesktopFileDlg::KURLDesktopFileDlg( const QString& textFileName, const QString& textUrl )
    : KDialogBase( Plain, QString::null, Ok|Cancel|User1, Ok, 0L /*parent*/, 0L, true,
                   true, KStdGuiItem::clear() )
{
    initDialog( textFileName, QString::null, textUrl, QString::null );
}

KURLDesktopFileDlg::KURLDesktopFileDlg( const QString& textFileName, const QString& textUrl, QWidget *parent )
    : KDialogBase( Plain, QString::null, Ok|Cancel|User1, Ok, parent, 0L, true,
                   true, KStdGuiItem::clear() )
{
    initDialog( textFileName, QString::null, textUrl, QString::null );
}

void KURLDesktopFileDlg::initDialog( const QString& textFileName, const QString& defaultName, const QString& textUrl, const QString& defaultUrl )
{
    QVBoxLayout * topLayout = new QVBoxLayout( plainPage(), 0, spacingHint() );

    // First line: filename
    QHBox * fileNameBox = new QHBox( plainPage() );
    topLayout->addWidget( fileNameBox );

    QLabel * label = new QLabel( textFileName, fileNameBox );
    m_leFileName = new KLineEdit( fileNameBox, 0L );
    m_leFileName->setMinimumWidth(m_leFileName->sizeHint().width() * 3);
    label->setBuddy(m_leFileName);  // please "scheck" style
    m_leFileName->setText( defaultName );
    m_leFileName->setSelection(0, m_leFileName->text().length()); // autoselect
    connect( m_leFileName, SIGNAL(textChanged(const QString&)),
             SLOT(slotNameTextChanged(const QString&)) );

    // Second line: url
    QHBox * urlBox = new QHBox( plainPage() );
    topLayout->addWidget( urlBox );
    label = new QLabel( textUrl, urlBox );
    m_urlRequester = new KURLRequester( defaultUrl, urlBox, "urlRequester" );
    m_urlRequester->setMode( KFile::File | KFile::Directory );
    
    m_urlRequester->setMinimumWidth( m_urlRequester->sizeHint().width() * 3 );
    topLayout->addWidget( m_urlRequester );
    connect( m_urlRequester->lineEdit(), SIGNAL(textChanged(const QString&)),
             SLOT(slotURLTextChanged(const QString&)) );
    label->setBuddy(m_urlRequester);  // please "scheck" style

    m_urlRequester->setFocus();
    enableButtonOK( !defaultName.isEmpty() && !defaultUrl.isEmpty() );
    connect( this, SIGNAL(user1Clicked()), this, SLOT(slotClear()) );
    m_fileNameEdited = false;
}

QString KURLDesktopFileDlg::url() const
{
    if ( result() == QDialog::Accepted )
        return m_urlRequester->url();
    else
        return QString::null;
}

QString KURLDesktopFileDlg::fileName() const
{
    if ( result() == QDialog::Accepted )
        return m_leFileName->text();
    else
        return QString::null;
}

void KURLDesktopFileDlg::slotClear()
{
    m_leFileName->setText( QString::null );
    m_urlRequester->clear();
    m_fileNameEdited = false;
}

void KURLDesktopFileDlg::slotNameTextChanged( const QString& )
{
    kdDebug() << k_funcinfo << endl;
    m_fileNameEdited = true;
    enableButtonOK( !m_leFileName->text().isEmpty() && !m_urlRequester->url().isEmpty() );
}

void KURLDesktopFileDlg::slotURLTextChanged( const QString& )
{
    if ( !m_fileNameEdited )
    {
        // use URL as default value for the filename
        // (we copy only its filename if protocol supports listing,
        // but for HTTP we don't want tons of index.html links)
        KURL url( m_urlRequester->url() );
        if ( KProtocolInfo::supportsListing( url ) )
            m_leFileName->setText( url.fileName() );
        else
            m_leFileName->setText( url.url() );
        m_fileNameEdited = false; // slotNameTextChanged set it to true erroneously
    }
    enableButtonOK( !m_leFileName->text().isEmpty() && !m_urlRequester->url().isEmpty() );
}


#include "knewmenu.moc"
