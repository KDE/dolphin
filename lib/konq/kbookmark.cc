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

#include "kbookmark.h"
#include "kbookmarkimporter.h"
#include <kdebug.h>
#include <kglobal.h>
#include <kmimetype.h>
#include <krun.h>
#include <kstddirs.h>
#include <kstringhandler.h>
#include <kurl.h>
#include <qdom.h>
#include <qfile.h>
#include <ksavefile.h>
#include <klineeditdlg.h>
#include <qtextstream.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <klocale.h>

KBookmarkManager* KBookmarkManager::s_pSelf = 0L;

KBookmarkManager* KBookmarkManager::self()
{
  if ( !s_pSelf )
    s_pSelf = new KBookmarkManager;

  return s_pSelf;
}

KBookmarkManager::KBookmarkManager( const QString & bookmarksFile, bool bImportDesktopFiles )
    : m_doc("BOOKMARKS")
{
    if ( s_pSelf )
        delete s_pSelf;
    s_pSelf = this;

    if (m_bookmarksFile.isEmpty())
        m_bookmarksFile = locateLocal("data", QString::fromLatin1("konqueror/bookmarks.xml"));
    else
        m_bookmarksFile = bookmarksFile;

    if ( !QFile::exists(m_bookmarksFile) )
    {
        // First time we use this class
        if ( m_doc.documentElement().isNull() )
        {
            QDomElement topLevel = m_doc.createElement("BOOKMARKS");
            m_doc.appendChild( topLevel );
        }
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
    QFile file( m_bookmarksFile );
    if ( !file.open( IO_ReadOnly ) )
    {
        kdWarning() << "Can't open " << m_bookmarksFile << endl;
        return;
    }
    m_doc.setContent( &file );

    QDomElement docElem = m_doc.documentElement(); // <BOOKMARKS>
    if ( docElem.isNull() )
        kdWarning() << "KBookmarkManager::parse : can't parse " << m_bookmarksFile << endl;

    file.close();
}

/* for debugging only
void KBookmarkManager::printGroup( QDomElement & group, int indent )
{
    QDomNode n = group.firstChild();
    while( !n.isNull() )
    {
        QDomElement e = n.toElement();
        if ( !e.isNull() )
            if ( e.tagName() == "TEXT" )
            {
                kdDebug() << "KBookmarkManager::printGroup text=" << e.text() << endl;
            }
            else if ( e.tagName() == "GROUP" )
                printGroup( e, indent+1 );
            else
                if ( e.tagName() == "BOOKMARK" )
                {
                    QString spaces;
                    for ( int i = 0 ; i < indent ; i++ )
                        spaces += " ";
                    kdDebug() << "URL=" << e.attribute("URL") << endl;
                    kdDebug() << spaces << e.text() << endl;
                }
                else
                    kdDebug() << "Unknown tag " << e.tagName() << endl;
        n = n.nextSibling();
    }
}
*/

void KBookmarkManager::importDesktopFiles()
{
    KBookmarkImporter importer( &m_doc );
    QString path(KGlobal::dirs()->saveLocation("data", "kfm/bookmarks", true));
    importer.import( path );
    //kdDebug() << m_doc.toCString() << endl;

    save();
}

bool KBookmarkManager::save()
{
    KSaveFile file( m_bookmarksFile );

    if ( file.status() != 0 )
    {
        KMessageBox::error( 0L, i18n("Couldn't save bookmarks in %1. %2").arg(m_bookmarksFile).arg(strerror(file.status())) );
        return false;
    }
    //This seems to save in local8bit ?!?!?!?
    //QTextStream ts( &file );
    //ts << m_doc;
    QCString cstr = m_doc.toCString(); // is in UTF8
    file.file()->writeBlock( cstr.data(), cstr.length() );
    if (!file.close())
    {
        KMessageBox::error( 0L, i18n("Couldn't save bookmarks in %1. %2").arg(m_bookmarksFile).arg(strerror(file.status())) );
        return false;
    }
    return true;
}

KBookmarkGroup KBookmarkManager::root()
{
    return KBookmarkGroup(m_doc.documentElement());
}

KBookmarkGroup KBookmarkManager::toolbar()
{
    return KBookmarkGroup(root().findToolbar());
}

void KBookmarkManager::emitChanged( KBookmarkGroup & group )
{
    save(); // The best time to save to disk is probably after each change.....

    // ######## TODO: tell the other processes too !
    emit changed( group );
}

void KBookmarkManager::slotEditBookmarks()
{
    KProcess proc;
    proc << QString::fromLatin1("keditbookmarks") << m_bookmarksFile;
    proc.start(KProcess::DontCare);
}

///////

KBookmark KBookmarkGroup::first() const
{
    QDomElement firstChild = element.firstChild().toElement();
    if ( firstChild.tagName() == "TEXT" )
        firstChild = firstChild.nextSibling().toElement();
    return KBookmark(firstChild);
}

KBookmark KBookmarkGroup::next( KBookmark & current ) const
{
    return KBookmark(current.element.nextSibling().toElement());
}

KBookmarkGroup KBookmarkGroup::createNewFolder( const QString & text )
{
    QString txt( text );
    if ( text.isEmpty() )
    {
        KLineEditDlg l( i18n("New Folder:"), "", 0L );
        l.setCaption( i18n("Create new bookmark folder in %1").arg( parentGroup().text() ) );
        if ( l.exec() )
            txt = l.text();
        else
            return KBookmarkGroup::null();
    }

    ASSERT(!element.isNull());
    QDomDocument doc = element.ownerDocument();
    QDomElement groupElem = doc.createElement( "GROUP" );
    element.appendChild( groupElem );
    QDomElement textElem = doc.createElement( "TEXT" );
    groupElem.appendChild( textElem );
    textElem.appendChild( doc.createTextNode( txt ) );
    KBookmarkManager::self()->emitChanged( *this );
    return KBookmarkGroup(groupElem);
}

void KBookmarkGroup::addBookmark( const QString & text, const QString & url )
{
    QDomDocument doc = element.ownerDocument();
    QDomElement elem = doc.createElement( "BOOKMARK" );
    element.appendChild( elem );
    elem.setAttribute( "URL", url );
    QString icon = KMimeType::iconForURL( KURL(url) );
    elem.setAttribute( "ICON", icon );
    elem.appendChild( doc.createTextNode( text ) );
    KBookmarkManager::self()->emitChanged( *this );
}

void KBookmarkGroup::deleteBookmark( KBookmark bk )
{
    element.removeChild( bk.element );
}

bool KBookmarkGroup::isToolbarGroup() const
{
    return ( element.attribute("TOOLBAR") == "1" );
}

QDomElement KBookmarkGroup::findToolbar() const
{
    // Search among the "GROUP" children only
    QDomNodeList list = element.elementsByTagName("GROUP");
    for ( uint i = 0 ; i < list.count() ; ++i )
    {
        QDomElement e = list.item(i).toElement();
        if ( e.attribute("TOOLBAR") == "1" )
            return e;
        else
        {
            QDomElement result = KBookmarkGroup(e).findToolbar();
            if (!result.isNull())
                return result;
        }
    }
    return QDomElement();
}

//////

bool KBookmark::isGroup() const
{
    return (element.tagName() == "GROUP"
            || element.tagName() == "BOOKMARKS" ); // don't forget the toplevel group
}

QString KBookmark::text() const
{
    return KStringHandler::csqueeze( fullText() );
}

QString KBookmark::fullText() const
{
    QString txt;
    // This small hack allows to avoid virtual tables in
    // KBookmark and KBookmarkGroup :)
    if (isGroup())
        txt = element.namedItem("TEXT").toElement().text();
    else
        txt = element.text();

    return txt;
}

QString KBookmark::url() const
{
    return element.attribute("URL");
}

QString KBookmark::icon() const
{
    QString icon = element.attribute("ICON");
    if ( icon.isEmpty() )
        // Default icon depends on URL for bookmarks, and is default directory
        // icon for groups.
        if ( isGroup() )
            icon = KMimeType::mimeType( "inode/directory" )->KMimeType::icon( QString::null, true );
        else
            icon = KMimeType::iconForURL( url() );
    return icon;
}

KBookmarkGroup KBookmark::parentGroup() const
{
    return KBookmarkGroup( element.parentNode().toElement() );
}

KBookmarkGroup KBookmark::toGroup() const
{
    ASSERT( isGroup() );
    return KBookmarkGroup(element);
}

//////////////

void KBookmarkOwner::openBookmarkURL(const QString& url)
{
  (void) new KRun(url);
}

#include "kbookmark.moc"
