/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>

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

#include <qdir.h>
#include <qpopupmenu.h>

#include <kaction.h>
#include <kapp.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kdirwatch.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>
#include <kurl.h>
#include <kprotocolinfo.h>
#include <kpopupmenu.h>

#include <kio/global.h>
#include <kio/job.h>

#include <kpropsdlg.h>
//#include <konq_dirwatcher_stub.h>
#include "konq_undo.h"
#include "knewmenu.h"

QValueList<KNewMenu::Entry> * KNewMenu::s_templatesList = 0L;
int KNewMenu::s_templatesVersion = 0;
bool KNewMenu::s_filesParsed = false;
KDirWatch * KNewMenu::s_pDirWatch = 0L;

KNewMenu::KNewMenu( KActionCollection * _collec, const char *name ) :
  KActionMenu( i18n( "Create &New" ), "filenew", _collec, name ), m_actionCollection( _collec ),
  menuItemsVersion( 0 )
{
    kdDebug(1203) << "KNewMenu::KNewMenu " << this << endl;
  // Don't fill the menu yet
  // We'll do that in slotCheckUpToDate (should be connected to abouttoshow)
}

KNewMenu::~KNewMenu()
{
    kdDebug(1203) << "KNewMenu::~KNewMenu " << this << endl;
}

void KNewMenu::slotCheckUpToDate( )
{
    kdDebug(1203) << "KNewMenu::slotCheckUpToDate() " << this
                  << " : menuItemsVersion=" << menuItemsVersion
                  << " s_templatesVersion=" << s_templatesVersion << endl;
    if (menuItemsVersion < s_templatesVersion || s_templatesVersion == 0)
    {
        kdDebug(1203) << "KNewMenu::slotCheckUpToDate() : recreating actions" << endl;
        // We need to clean up the action collection
        // We look for our actions using the group
        QValueList<KAction*> actions = m_actionCollection->actions( "KNewMenu" );
        for( QValueListIterator<KAction*> it = actions.begin(); it != actions.end(); ++it )
        {
            remove( *it );
            m_actionCollection->remove( *it );
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
    kdDebug(1203) << "KNewMenu::parseFiles()" << endl;
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
                    templatePath = config.readEntry("URL");
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
    kdDebug(1203) << "KNewMenu::fillMenu()" << endl;
    popupMenu()->clear();
    //KAction * act = new KAction( i18n( "Folder" ), 0, this, SLOT( slotNewFile() ),
    //                              m_actionCollection, QString("newmenu1") );

    //act->setGroup( "KNewMenu" );
    //act->plug( popupMenu() );

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

            QValueList<KAction*> actions = m_actionCollection->actions();
            QValueListIterator<KAction*> it = actions.begin();
            for( ; it != actions.end() && !bSkip; ++it )
            {
                if ( (*it)->text() == (*templ).text )
                {
                    kdDebug(1203) << "skipping " << (*templ).filePath << endl;
                    bSkip = true;
                }
            }

            if ( !bSkip )
            {
                KAction * act = new KAction( (*templ).text+"...", (*templ).icon, 0, this, SLOT( slotNewFile() ),
                                             m_actionCollection, QCString().sprintf("newmenu%d", i ) );
                act->setGroup( "KNewMenu" );
                act->plug( popupMenu() );
            }
        } else { // Separate system from personal templates
            ASSERT( (*templ).entryType != 0 );

            KActionSeparator * act = new KActionSeparator();
            act->plug( popupMenu() );
        }
    }
}

void KNewMenu::slotFillTemplates()
{
    kdDebug(1203) << "KNewMenu::slotFillTemplates()" << endl;
    // Ensure any changes in the templates dir will call this
    if ( ! s_pDirWatch )
    {
        s_pDirWatch = new KDirWatch( 5000 ); // 5 seconds
        QStringList dirs = m_actionCollection->instance()->dirs()->resourceDirs("templates");
        for ( QStringList::Iterator it = dirs.begin() ; it != dirs.end() ; ++it )
        {
            kdDebug(1203) << "Templates resource dir: " << *it << endl;
            s_pDirWatch->addDir( *it );
        }
        connect ( s_pDirWatch, SIGNAL( dirty( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        connect ( s_pDirWatch, SIGNAL( fileDirty( const QString & ) ),
                  this, SLOT ( slotFillTemplates() ) );
        // Ok, this doesn't cope with new dirs in KDEDIRS, but that's another story
    }
    s_templatesVersion++;
    s_filesParsed = false;

    s_templatesList->clear();
    //s_templatesList->append( "Folder" );

    // Look into "templates" dirs.
    QStringList files = m_actionCollection->instance()->dirs()->findAllResources("templates");
    for ( QStringList::Iterator it = files.begin() ; it != files.end() ; ++it )
    {
        kdDebug(1203) << *it << endl;
        if ( (*it)[0] != '.' )
        {
            Entry e;
            e.filePath = *it;
            e.entryType = 0; // not parsed yet
            s_templatesList->append( e );
        }
    }
}

void KNewMenu::slotNewFile()
{
    int id = QString( sender()->name() + 7 ).toInt(); // skip "newmenu"
    if (id == 0) return;

    Entry entry = *(s_templatesList->at( id - 1 ));
    //kdDebug(1203) << QString("sFile = %1").arg(sFile) << endl;

    //if ( sName != "Folder" ) {
    if ( !QFile::exists( entry.templatePath ) ) {
          kdWarning(1203) << entry.templatePath << " doesn't exist" << endl;
          KMessageBox::sorry( 0L, i18n("The templates file %1 doesn't exist!").arg(entry.templatePath));
          return;
    }
    //not i18n'ed enough said Hans
    //QString defaultName = KURL( entry.templatePath ).fileName();
    QString defaultName = entry.text;
    m_isURLDesktopFile = false;
    if ( KDesktopFile::isDesktopFile( entry.templatePath ) )
    {
        KDesktopFile df( entry.templatePath );
        kdDebug(1203) <<  df.readType() << endl;
        if ( df.readType() == "Link" )
        {
            m_isURLDesktopFile = true;
            defaultName = df.readURL();
        }
        else
        {
          KURL::List::Iterator it = popupFiles.begin();
          for ( ; it != popupFiles.end(); ++it )
          {
              //kdDebug(1203) << "first arg=" << entry.templatePath << endl;
              //kdDebug(1203) << "second arg=" << (*it).url() << endl;
              //kdDebug(1203) << "third arg=" << defaultName << endl;
              (void) new KPropertiesDialog( entry.templatePath, *it, defaultName );
          }
          return; // done, exit.
        }
    }

    // The template is not a desktop file [or a URL one]
    // Show the small dialog for getting the destination filename
    KLineEditDlg l( entry.comment, defaultName, 0L );
    if ( l.exec() )
    {
        QString name = l.text();
        if ( name.length() == 0 )
            return;

        if ( m_isURLDesktopFile )
        {
          m_destURL = name;
          name += ".desktop";
        }

        KURL::List::Iterator it = popupFiles.begin();
        /*
        if ( sFile =="Folder" )
        {
            for ( ; it != popupFiles.end(); ++it )
            {
              QString url = (*it).path(1) + KIO::encodeFileName(name);
              KURL::encode(url); // hopefully will disappear with next KURL
              KIO::Job * job = KIO::mkdir( url );
              connect( job, SIGNAL( result( KIO::Job * ) ),
                       SLOT( slotResult( KIO::Job * ) ) );
            }
        }
        else
        {
        */
            QString src = entry.templatePath; // KGlobalSettings::templatesPath() + sFile;
            for ( ; it != popupFiles.end(); ++it )
            {
                KURL dest( *it );
                dest.addPath( KIO::encodeFileName(name) ); // Chosen destination file name

                KURL uSrc;
                uSrc.setPath( src );
                //kdDebug(1203) << "KNewMenu : KIO::copyAs( " << uSrc.url() << ", " << dest.url() << ")" << endl;
                KIO::Job * job = KIO::copyAs( uSrc, dest );
                connect( job, SIGNAL( result( KIO::Job * ) ),
                         SLOT( slotResult( KIO::Job * ) ) );
                KURL::List lst;
                lst.append( uSrc );
                (void)new KonqCommandRecorder( KonqCommand::COPY, lst, dest, job );
            }
        //}
    }
}

void KNewMenu::slotResult( KIO::Job * job )
{
    if (job->error())
        job->showErrorDialog();
    else
      if ( m_isURLDesktopFile )
      {
        KURL destURL = static_cast<KIO::CopyJob*>(job)->destURL();
        if ( destURL.isLocalFile() )
        {
          //kdDebug(1203) << destURL.path() << endl;
          KDesktopFile df( destURL.path() );
          df.writeEntry( "Icon", KProtocolInfo::icon( KURL(m_destURL).protocol() ) );
          df.writeEntry( "URL", m_destURL );
          df.sync();
        }
      }
}

#include "knewmenu.moc"
