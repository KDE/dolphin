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

#include "kbookmarkimporter.h"
#include <kstddirs.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kstringhandler.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <qdir.h>
#include <qtextcodec.h>
#include <kio/global.h>

#include <sys/types.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/stat.h>

////////////////////

void KBookmarkImporter::import( const QString & path )
{
    QDomElement elem = m_pDoc->documentElement();
    ASSERT(!elem.isNull());
    scanIntern( elem, path );
}

void KBookmarkImporter::scanIntern( QDomElement & parentElem, const QString & _path )
{
    kdDebug(1203) << "KBookmarkImporter::scanIntern " << _path << endl;
    // Substitute all symbolic links in the path
    QDir dir( _path );
    QString canonical = dir.canonicalPath();

    if ( m_lstParsedDirs.contains(canonical) )
    {
        kdWarning() << "Directory " << canonical << " already parsed" << endl;
        return;
    }

    m_lstParsedDirs.append( canonical );

    DIR *dp;
    struct dirent *ep;
    dp = opendir( QFile::encodeName(_path) );
    if ( dp == 0L )
        return;

    // Loop thru all directory entries
    while ( ( ep = readdir( dp ) ) != 0L )
    {
        if ( strcmp( ep->d_name, "." ) != 0 && strcmp( ep->d_name, ".." ) != 0 )
        {
            KURL file;
            file.setPath( QString( _path ) + '/' + QFile::decodeName(ep->d_name) );

            KMimeType::Ptr res = KMimeType::findByURL( file, 0, true );
            //kdDebug(1203) << " - " << file.url() << "  ->  " << res->name() << endl;

            if ( res->name() == "inode/directory" )
            {
                // We could use KBookmarkGroup::createNewFolder, but then it
                // would notify about the change, so we'd need a flag, etc.
                QDomElement groupElem = m_pDoc->createElement( "folder" );
                parentElem.appendChild( groupElem );
                QDomElement textElem = m_pDoc->createElement( "title" );
                groupElem.appendChild( textElem );
                textElem.appendChild( m_pDoc->createTextNode( KIO::decodeFileName( ep->d_name ) ) );
                if ( KIO::decodeFileName( ep->d_name ) == "Toolbar" )
                    groupElem.setAttribute("toolbar","yes");
                scanIntern( groupElem, file.path() );
            }
            else if ( res->name() == "application/x-desktop" )
            {
                KSimpleConfig cfg( file.path(), true );
                cfg.setDesktopGroup();
                QString type = cfg.readEntry( "Type" );
                // Is it really a bookmark file ?
                if ( type == "Link" )
                    parseBookmark( parentElem, ep->d_name, cfg, 0 /* desktop group */ );
                else
                    kdWarning(1203) << "  Not a link ? Type=" << type << endl;
            }
            else if ( res->name() == "text/plain")
            {
                // maybe its an IE Favourite..
                KSimpleConfig cfg( file.path(), true );
                QStringList grp = cfg.groupList().grep( "internetshortcut", false );
                if ( grp.count() == 0 )
                    continue;
                cfg.setGroup( *grp.begin() );

                QString url = cfg.readEntry("URL");
                if (!url.isEmpty() )
                    parseBookmark( parentElem, ep->d_name, cfg, *grp.begin() );
            } else
                kdWarning(1203) << "Invalid bookmark : found mimetype='" << res->name() << "' for file='" << file.path() << "'!" << endl;
        }
    }

    closedir( dp );
}

void KBookmarkImporter::parseBookmark( QDomElement & parentElem, QCString _text,
                                       KSimpleConfig& _cfg, const QString &_group )
{
    if ( !_group.isEmpty() )
        _cfg.setGroup( _group );
    else
        _cfg.setDesktopGroup();

    QString url = _cfg.readEntry( "URL" );
    QString icon = _cfg.readEntry( "Icon", QString::null );
    if (icon.right( 4 ) == ".xpm" ) // prevent warnings
        icon.truncate( icon.length() - 4 );

    QString text = KIO::decodeFileName( QString::fromLocal8Bit(_text) );
    if ( text.length() > 8 && text.right( 8 ) == ".desktop" )
        text.truncate( text.length() - 8 );
    if ( text.length() > 7 && text.right( 7 ) == ".kdelnk" )
        text.truncate( text.length() - 7 );

    QDomElement elem = m_pDoc->createElement( "bookmark" );
    parentElem.appendChild( elem );
    elem.setAttribute( "href", url );
    //if ( icon != "www" ) // No need to save the default
    // Hmm, after all, it makes KBookmark::pixmapFile faster,
    // and it shows a nice feature to those reading the file
    elem.setAttribute( "icon", icon );
    QDomElement textElem = m_pDoc->createElement( "title" );
    elem.appendChild( textElem );
    textElem.appendChild( m_pDoc->createTextNode( text ) );
    kdDebug(1203) << "KBookmarkImporter::parseBookmark text=" << text << endl;
}

////

void KNSBookmarkImporter::parseNSBookmarks( bool utf8 )
{
    QFile f(m_fileName);
    QRegExp amp("&amp;");
    QRegExp lt("&lt;");
    QRegExp gt("&gt;");
    QTextCodec * codec = utf8 ? QTextCodec::codecForName("UTF-8") : QTextCodec::codecForLocale();
    ASSERT(codec);
    if (!codec)
        return;

    if(f.open(IO_ReadOnly)) {

#define NSBKLINELIMIT 2048
        QCString s(NSBKLINELIMIT);
        // skip header
        while(f.readLine(s.data(), NSBKLINELIMIT) >= 0 && !s.contains("<DL>"));

        while(f.readLine(s.data(), NSBKLINELIMIT)>=0) {
            if ( s[s.length()-1] != '\n' ) // Gosh, this line is longer than NSBKLINELIMIT. Skipping.
            {
               kdWarning() << "Netscape bookmarks contain a line longer than " << NSBKLINELIMIT << ". Skipping." << endl;
               continue;
            }
            QCString t = s.stripWhiteSpace();
            if(t.left(12).upper() == "<DT><A HREF=" ||
               t.left(16).upper() == "<DT><H3><A HREF=") {
              int firstQuotes = t.find('"')+1;
              int secondQuotes = t.find('"', firstQuotes);
              if (firstQuotes != -1 && secondQuotes != -1)
              {
                QCString link = t.mid(firstQuotes, secondQuotes-firstQuotes);
                int endTag = t.find('>', secondQuotes+1);
                QCString name = t.mid(endTag+1);
                name = name.left(name.findRev('<'));
                if ( name.right(4) == "</A>" )
                    name = name.left( name.length() - 4 );
                name.replace( amp, "&" ).replace( lt, "<" ).replace( gt, ">" );
                QCString additionnalInfo = t.mid( secondQuotes+1, endTag-secondQuotes-1 );

                emit newBookmark( KStringHandler::csqueeze(codec->toUnicode(name)),
                                  link, codec->toUnicode(additionnalInfo) );
              }
            }
            else if(t.left(7).upper() == "<DT><H3") {
                int endTag = t.find('>', 7);
                QCString name = t.mid(endTag+1);
                name = name.left(name.findRev('<'));
                name.replace( amp, "&" ).replace( lt, "<" ).replace( gt, ">" );
                QCString additionnalInfo = t.mid( 8, endTag-8 );
                bool folded = (additionnalInfo.left(6) == "FOLDED");
                if (folded) additionnalInfo.remove(0,7);

                emit newFolder( KStringHandler::csqueeze(codec->toUnicode(name)),
                                !folded,
                                codec->toUnicode(additionnalInfo) );
            }
            else if(t.left(4).upper() == "<HR>")
                emit newSeparator();
            else if(t.left(8).upper() == "</DL><P>")
                emit endMenu();
        }

        f.close();
    }
}

QString KNSBookmarkImporter::netscapeBookmarksFile( bool )
{
    return QDir::homeDirPath() + "/.netscape/bookmarks.html";
}

QString KNSBookmarkImporter::mozillaBookmarksFile( bool forSaving )
{
    //return QDir::homeDirPath() + "/.mozilla/default/bookmarks.html";
    if ( forSaving )
        return KFileDialog::getSaveFileName( QDir::homeDirPath() + "/.mozilla",
                                             i18n("*.html|HTML files (*.html)") );
    else
        return KFileDialog::getOpenFileName( QDir::homeDirPath() + "/.mozilla",
                                             i18n("*.html|HTML files (*.html)") );
}

#include "kbookmarkimporter.moc"
