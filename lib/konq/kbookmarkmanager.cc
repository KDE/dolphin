/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

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

#include "kbookmarkmanager.h"
#include "kbookmarkimporter.h"
#include <kdebug.h>
#include <kglobal.h>
#include <kmimetype.h>
#include <krun.h>
#include <kstddirs.h>
#include <kurl.h>
#include <qfile.h>
#include <ksavefile.h>
#include <qtextstream.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <klocale.h>
#include <kapp.h>
#include <dcopclient.h>

KBookmarkManager* KBookmarkManager::s_pSelf = 0L;

KBookmarkManager* KBookmarkManager::self()
{
  if ( !s_pSelf )
    s_pSelf = new KBookmarkManager;

  return s_pSelf;
}

KBookmarkManager::KBookmarkManager( const QString & bookmarksFile, bool bImportDesktopFiles )
    : DCOPObject("KBookmarkManager"), m_doc("xbel")
{
    if ( s_pSelf )
        delete s_pSelf;
    s_pSelf = this;

    if (bookmarksFile.isEmpty())
        m_bookmarksFile = locateLocal("data", QString::fromLatin1("konqueror/bookmarks.xml"));
    else
        m_bookmarksFile = bookmarksFile;

    if ( !QFile::exists(m_bookmarksFile) )
    {
        // First time we use this class
        QDomElement topLevel = m_doc.createElement("xbel");
        m_doc.appendChild( topLevel );
        if ( bImportDesktopFiles )
            importDesktopFiles();
    }
    else
    {
        parse();
    }
}

KBookmarkManager::~KBookmarkManager()
{
    s_pSelf = 0L;
}

void KBookmarkManager::parse()
{
    //kdDebug(1203) << "KBookmarkManager::parse " << m_bookmarksFile << endl;
    QFile file( m_bookmarksFile );
    if ( !file.open( IO_ReadOnly ) )
    {
        kdWarning() << "Can't open " << m_bookmarksFile << endl;
        return;
    }
    m_doc = QDomDocument("xbel");
    m_doc.setContent( &file );

    QDomElement docElem = m_doc.documentElement();
    if ( docElem.isNull() )
        kdWarning() << "KBookmarkManager::parse : can't parse " << m_bookmarksFile << endl;
    else
    {
        QString mainTag = docElem.tagName();
        if ( mainTag == "BOOKMARKS" )
        {
            kdWarning() << "Old style bookmarks found. Calling convertToXBEL." << endl;
            docElem.setTagName("xbel");
            if ( docElem.hasAttribute( "HIDE_NSBK" ) ) // non standard either, but we need it
            {
                docElem.setAttribute( "hide_nsbk", docElem.attribute( "HIDE_NSBK" ) == "1" ? "yes" : "no" );
                docElem.removeAttribute( "HIDE_NSBK" );
            }

            convertToXBEL( docElem );
            save();
        }
        else if ( mainTag != "xbel" )
            kdWarning() << "KBookmarkManager::parse : unknown main tag " << mainTag << endl;
    }

    file.close();
}

void KBookmarkManager::convertToXBEL( QDomElement & group )
{
    QDomNode n = group.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement();
        if ( !e.isNull() )
            if ( e.tagName() == "TEXT" )
            {
                e.setTagName("title");
            }
            else if ( e.tagName() == "SEPARATOR" )
            {
                e.setTagName("separator"); // so close...
            }
            else if ( e.tagName() == "GROUP" )
            {
                e.setTagName("folder");
                convertAttribute(e, "ICON","icon"); // non standard, but we need it
                if ( e.hasAttribute( "TOOLBAR" ) ) // non standard either, but we need it
                {
                    e.setAttribute( "toolbar", e.attribute( "TOOLBAR" ) == "1" ? "yes" : "no" );
                    e.removeAttribute( "TOOLBAR" );
                }

                convertAttribute(e, "NETSCAPEINFO","netscapeinfo"); // idem
                bool open = (e.attribute("OPEN") == "1");
                e.removeAttribute("OPEN");
                e.setAttribute("folded", open ? "no" : "yes");
                convertToXBEL( e );
            }
            else
                if ( e.tagName() == "BOOKMARK" )
                {
                    e.setTagName("bookmark"); // so much difference :-)
                    convertAttribute(e, "ICON","icon"); // non standard, but we need it
                    convertAttribute(e, "NETSCAPEINFO","netscapeinfo"); // idem
                    convertAttribute(e, "URL","href");
                    QString text = e.text();
                    while ( !e.firstChild().isNull() ) // clean up the old contained text
                        e.removeChild(e.firstChild());
                    QDomElement titleElem = e.ownerDocument().createElement("title");
                    e.appendChild( titleElem ); // should be the only child anyway
                    titleElem.appendChild( e.ownerDocument().createTextNode( text ) );
                }
                else
                    kdWarning(1203) << "Unknown tag " << e.tagName() << endl;
        n = n.nextSibling();
    }
}

void KBookmarkManager::convertAttribute( QDomElement elem, const QString & oldName, const QString & newName )
{
    if ( elem.hasAttribute( oldName ) )
    {
        elem.setAttribute( newName, elem.attribute( oldName ) );
        elem.removeAttribute( oldName );
    }
}

void KBookmarkManager::importDesktopFiles()
{
    KBookmarkImporter importer( &m_doc );
    QString path(KGlobal::dirs()->saveLocation("data", "kfm/bookmarks", true));
    importer.import( path );
    //kdDebug(1203) << m_doc.toCString() << endl;

    save();
}

bool KBookmarkManager::save()
{
    //kdDebug(1203) << "KBookmarkManager::save " << m_bookmarksFile << endl;
    KSaveFile file( m_bookmarksFile );

    if ( file.status() != 0 )
    {
        KMessageBox::error( 0L, i18n("Couldn't save bookmarks in %1. %2").arg(m_bookmarksFile).arg(strerror(file.status())) );
        return false;
    }
    QCString cstr = m_doc.toCString(); // is in UTF8
    file.file()->writeBlock( cstr.data(), cstr.length() );
    if (!file.close())
    {
        KMessageBox::error( 0L, i18n("Couldn't save bookmarks in %1. %2").arg(m_bookmarksFile).arg(strerror(file.status())) );
        return false;
    }
    return true;
}

KBookmarkGroup KBookmarkManager::root() const
{
    return KBookmarkGroup(m_doc.documentElement());
}

KBookmarkGroup KBookmarkManager::toolbar()
{
    QDomElement elem = root().findToolbar();
    if (elem.isNull())
        return root(); // Root is the bookmark toolbar if none has been set.
    else
        return KBookmarkGroup(root().findToolbar());
}

KBookmark KBookmarkManager::findByAddress( const QString & address )
{
    //kdDebug(1203) << "KBookmarkManager::findByAddress " << address << endl;
    KBookmark result = root();
    // The address is something like /5/10/2
    QStringList addresses = QStringList::split('/',address);
    for ( QStringList::Iterator it = addresses.begin() ; it != addresses.end() ; ++it )
    {
        uint number = (*it).toUInt();
        //kdDebug(1203) << "KBookmarkManager::findByAddress " << number << endl;
        ASSERT(result.isGroup());
        KBookmarkGroup group = result.toGroup();
        KBookmark bk = group.first();
        for ( uint i = 0 ; i < number ; ++i )
            bk = group.next(bk);
        ASSERT(!bk.isNull());
        result = bk;
    }
    if (result.isNull())
        kdWarning() << "KBookmarkManager::findByAddress: couldn't find item " << address << endl;
    return result;
}

void KBookmarkManager::emitChanged( KBookmarkGroup & group )
{
    save();

    // Tell the other processes too
    //kdDebug(1203) << "KBookmarkManager::emitChanged : broadcasting change " << group.address() << endl;
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << group.address();
    kapp->dcopClient()->send( "*", "KBookmarkManager", "notifyChanged(QString)", data );

    // We do get our own broadcast, so no need for this anymore
    //emit changed( group );
}

void KBookmarkManager::notifyCompleteChange( QString caller )
{
    //kdDebug(1203) << "KBookmarkManager::notifyCompleteChange" << endl;
    // The bk editor tells us we should reload everything
    // Reparse
    parse();
    // Tell our GUI
    // (emit with group == "" to directly mark the root menu as dirty)
    emit changed( "", caller );
    // Also tell specifically about the toolbar - unless it's the root as well
    KBookmarkGroup tbGroup = toolbar();
    if (!tbGroup.isNull() && !tbGroup.groupAddress().isEmpty())
        emit changed( tbGroup.groupAddress(), caller );
}

void KBookmarkManager::notifyChanged( QString groupAddress ) // DCOP call
{
    // Reparse (the whole file, no other choice)
    // Of course, if we are the emitter this is a bit stupid....
    parse();

    //kdDebug(1203) << "KBookmarkManager::notifyChanged " << groupAddress << endl;
    //KBookmarkGroup group = findByAddress( groupAddress ).toGroup();
    //ASSERT(!group.isNull());
    emit changed( groupAddress, QString::null );
}

bool KBookmarkManager::showNSBookmarks() const
{
    // The attr name is HIDE, so that the default is to show them
    return root().internalElement().attribute("hide_nsbk") != "yes";
}

void KBookmarkManager::slotEditBookmarks()
{
    KProcess proc;
    proc << QString::fromLatin1("keditbookmarks") << m_bookmarksFile;
    proc.start(KProcess::DontCare);
}

///////

void KBookmarkOwner::openBookmarkURL(const QString& url)
{
  (void) new KRun(url);
}

#include "kbookmarkmanager.moc"
