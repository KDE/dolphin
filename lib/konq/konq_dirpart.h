/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konqdirpart_h
#define __konqdirpart_h

#include <qstring.h>
#include <kparts/part.h>

namespace KParts { class BrowserExtension; }
class KonqPropsView;
class QScrollView;
class KFileItem;

class KonqDirPart: public KParts::ReadOnlyPart
{
  Q_OBJECT
public:
    KonqDirPart( QObject *parent, const char *name );
    virtual ~KonqDirPart();

    /**
     * The derived part should call this in its constructor
     */
    void setBrowserExtension( KParts::BrowserExtension * extension )
      { m_extension = extension; }
    KParts::BrowserExtension * extension()
      { return m_extension; }

    QScrollView * scrollWidget();

    virtual void saveState( QDataStream &stream );
    virtual void restoreState( QDataStream &stream );

    void mmbClicked( KFileItem * fileItem );

    void setNameFilter( const QString & nameFilter ) { m_nameFilter = nameFilter; }
    QString nameFilter() const { return m_nameFilter; }

    KonqPropsView * props() const { return m_pProps; }

    /**
     * "Cut" icons : disable those whose URL is in lst, enable the others
     */
    virtual void disableIcons( const QStrList & lst ) = 0;

    /**
     * Helper for statusbar information about selection
     */
    static QString displayString(int items, int files, long size, int dirs);

public slots:
    void slotBackgroundColor();
    void slotBackgroundImage();
    /**
     * Called when the clipboard's data changes, to update the 'cut' icons
     * Call this when the directory's listing is finished, to draw icons as cut.
     */
    void slotClipboardDataChanged();

protected:
    QString m_nameFilter;

    KParts::BrowserExtension * m_extension;
    /**
     * View properties
     */
    KonqPropsView * m_pProps;
};

#endif
