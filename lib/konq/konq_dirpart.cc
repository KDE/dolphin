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

#include "konq_dirpart.h"
#include "konq_bgnddlg.h"
#include "konq_propsview.h"

#include <kcolordlg.h>
#include <kdebug.h>

KonqDirPart::KonqDirPart( QObject *parent, const char *name )
  : KParts::ReadOnlyPart( parent, name ),
    m_pProps( 0L )
{
}

KonqDirPart::~KonqDirPart()
{
}

QScrollView * KonqDirPart::scrollWidget()
{
  return static_cast<QScrollView *>(widget());
}

void KonqDirPart::slotBackgroundColor()
{
    QColor bgndColor = m_pProps->bgColor( widget() );
    if ( KColorDialog::getColor( bgndColor ) == KColorDialog::Accepted )
    {
	m_pProps->setBgColor( bgndColor );
	m_pProps->setBgPixmapFile( "" );
        m_pProps->applyColors( widget() );
	widget()->repaint();
    }
}

void KonqDirPart::slotBackgroundImage()
{
    KonqBgndDialog dlg( m_pProps->bgPixmapFile(), instance() );
    if ( dlg.exec() == KonqBgndDialog::Accepted )
    {
	m_pProps->setBgPixmapFile( dlg.pixmapFile() );
        m_pProps->applyColors( widget() );
	widget()->repaint();
    }
}

void KonqDirPart::saveState( QDataStream &stream )
{
  kdDebug() << "void KonqDirPart::saveState( QDataStream &stream )" << endl;
  stream << m_nameFilter;
}

void KonqDirPart::restoreState( QDataStream &stream )
{
  // Warning: see comment in IconViewBrowserExtension::restoreState about order
  kdDebug() << "void KonqDirPart::restoreState( QDataStream &stream )" << endl;
  stream >> m_nameFilter;
}

#include "konq_dirpart.moc"
