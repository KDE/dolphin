/* This file is part of the KDE project
   Copyright (C) 2002 Rolf Magnus <ramagnus@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation version 2.0

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   $Id$
*/

#include "konq_listview.h"
#include <konq_settings.h>
#include <kfilemetainfo.h>
#include <kdebug.h>
#include <klocale.h>
#include <assert.h>
#include <stdio.h>
#include <qpainter.h>
#include <kiconloader.h>
#include "konq_infolistviewitem.h"
#include "konq_infolistviewwidget.h"

/**************************************************************
 *
 * KonqInfoListViewItem
 *
 **************************************************************/
KonqInfoListViewItem::KonqInfoListViewItem( KonqInfoListViewWidget *_widget, KonqInfoListViewItem * _parent, KFileItem* _fileitem )
:KonqBaseListViewItem( _widget,_parent,_fileitem ), m_ILVWidget(_widget)
{
    updateContents();
}

KonqInfoListViewItem::KonqInfoListViewItem( KonqInfoListViewWidget *_listViewWidget, KFileItem* _fileitem )
:KonqBaseListViewItem(_listViewWidget,_fileitem), m_ILVWidget(_listViewWidget)
{
   updateContents();
}

void KonqInfoListViewItem::updateContents()
{
    // Set the pixmap
    setDisabled( m_bDisabled );

    // Set the text of each column
    setText(0,m_fileitem->text());
   
#if 0
   if (S_ISDIR(m_fileitem->mode()))
       sortChar='0';
   //now we have the first column, so let's do the rest

   for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
   {
      ColumnInfo *tmpColumn=&static_cast<KonqBaseListViewWidget *>(listView())->columnConfigInfo()[i];
      if (tmpColumn->displayThisOne)
      {
         switch (tmpColumn->udsId)
         {
         case KIO::UDS_USER:
            setText(tmpColumn->displayInColumn,m_fileitem->user());
            break;
         case KIO::UDS_GROUP:
            setText(tmpColumn->displayInColumn,m_fileitem->group());
            break;
         case KIO::UDS_FILE_TYPE:
            setText(tmpColumn->displayInColumn,m_fileitem->mimeComment());
            break;
         case KIO::UDS_MIME_TYPE:
            setText(tmpColumn->displayInColumn,m_fileitem->mimetype());
            break;
         case KIO::UDS_URL:
            setText(tmpColumn->displayInColumn,m_fileitem->url().prettyURL());
            break;
         case KIO::UDS_LINK_DEST:
            setText(tmpColumn->displayInColumn,m_fileitem->linkDest());
            break;
         case KIO::UDS_SIZE:
            if ( static_cast<KonqBaseListViewWidget *>(listView())->m_pSettings->fileSizeInBytes() )
                setText(tmpColumn->displayInColumn,KGlobal::locale()->formatNumber( m_fileitem->size(),0)+" ");
            else
                setText(tmpColumn->displayInColumn,KIO::convertSize(m_fileitem->size())+" ");
            break;
         case KIO::UDS_ACCESS:
            setText(tmpColumn->displayInColumn,makeAccessString(m_fileitem->permissions()));
            break;
         case KIO::UDS_MODIFICATION_TIME:
         case KIO::UDS_ACCESS_TIME:
         case KIO::UDS_CREATION_TIME:
            {
               QDateTime dt;
               time_t _time = m_fileitem->time( tmpColumn->udsId );
               if ( _time != 0 )
               {
                   dt.setTime_t( _time );
                   setText(tmpColumn->displayInColumn,KGlobal::locale()->formatDate(dt.date(),TRUE)+" "+KGlobal::locale()->formatTime(dt.time())+" ");
                   //setText(tmpColumn->displayInColumn,KGlobal::locale()->formatDateTime(dt));
               }
            }
            break;
         default:
            break;
         };
      };
   };
#endif
}

void KonqInfoListViewItem::gotMetaInfo()
{
    KFileMetaInfo info = item()->metaInfo(false);

    if (!info.isValid()) return;

    QStringList::ConstIterator it = m_ILVWidget->columnKeys().begin();
    for (int i = 1; it != m_ILVWidget->columnKeys().end(); ++it, ++i)
    {
        KFileMetaInfoItem kfmii = info.item(*it);

        m_columnTypes.append(kfmii.type());

        if (!kfmii.isValid())
            continue;

        QString s = kfmii.string().simplifyWhiteSpace();
        setText(i, s.isNull() ? QString("") : s);
    }
}  

int KonqInfoListViewItem::compare( QListViewItem *item, int col, bool ascending ) const
{
    KonqInfoListViewItem *i = dynamic_cast<KonqInfoListViewItem *>(item);

    if(!i ||
       int(m_columnTypes.size()) <= col ||
       int(i->m_columnTypes.size()) <= col ||
       m_columnTypes[col] != i->m_columnTypes[col])
    {
        return KonqBaseListViewItem::compare(item, col, ascending);
    }

    bool ok1;
    int value1 = text(col).toInt(&ok1);

    bool ok2;
    int value2 = i->text(col).toInt(&ok2);

    if(!ok1 || !ok2)
    {
        return KonqBaseListViewItem::compare(item, col, ascending);
    }

    if(value1 == value2)
    {
        return 0;
    }

    return value1 > value2 ? 1 : -1;
}

void KonqInfoListViewItem::setDisabled( bool disabled )
{
    KonqBaseListViewItem::setDisabled( disabled );
    int iconSize = static_cast<KonqBaseListViewWidget *>(listView())->iconSize();
    iconSize = iconSize ? iconSize : KGlobal::iconLoader()->currentSize( KIcon::Small ); // Default = small
    setPixmap( 0, m_fileitem->pixmap( iconSize, state() ) );
}

void KonqInfoListViewItem::paintCell( QPainter *_painter, const QColorGroup & _cg, int _column, int _width, int _alignment )
{
  QColorGroup cg( _cg );

  if ( _column == 0 )
  {
     _painter->setFont( static_cast<KonqBaseListViewWidget *>(listView())->itemFont() );
  }
  //else
  //   _painter->setPen( static_cast<KonqBaseListViewWidget *>(listView())->color() );

  cg.setColor( QColorGroup::Text, static_cast<KonqBaseListViewWidget *>(listView())->itemColor() );

  KListViewItem::paintCell( _painter, cg, _column, _width, _alignment );
}

#if 0
void KonqBaseListViewItem::mimetypeFound()
{
    // Update icon
    setDisabled( m_bDisabled );
    uint done = 0;
    KonqBaseListViewWidget * lv = static_cast<KonqBaseListViewWidget*>(listView());
    for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms && done < 2; i++)
    {
        ColumnInfo *tmpColumn=&lv->columnConfigInfo()[i];
        if (lv->columnConfigInfo()[i].udsId==KIO::UDS_FILE_TYPE && tmpColumn->displayThisOne)
        {
            setText(tmpColumn->displayInColumn, m_fileitem->mimeComment());
            done++;
        }
        if (lv->columnConfigInfo()[i].udsId==KIO::UDS_MIME_TYPE && tmpColumn->displayThisOne)
        {
            setText(tmpColumn->displayInColumn, m_fileitem->mimetype());
            done++;
        }
    }
}

#endif
