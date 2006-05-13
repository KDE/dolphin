/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2000, 2001, 2002 David Faure <faure@kde.org>
   Copyright (C) 2004 Martin Koller <m.koller@surfeu.at>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_FILETIP_H
#define KONQ_FILETIP_H

#include <q3frame.h>
#include <QPixmap>
//Added by qt3to4:
#include <QLabel>
#include <QResizeEvent>
#include <QEvent>
#include <kio/previewjob.h>

#include <libkonq_export.h>

class KFileItem;
class QLabel;
class Q3ScrollView;
class QTimer;

//--------------------------------------------------------------------------------

class LIBKONQ_EXPORT KonqFileTip : public Q3Frame
{
  Q_OBJECT

  public:
    KonqFileTip( Q3ScrollView *parent );
    ~KonqFileTip();

    void setPreview(bool on);

    /**
      @param on show tooltip at all
      @param preview include file preview in tooltip
      @param num the number of tooltip texts to get from KFileItem
      */
    void setOptions( bool on, bool preview, int num );

    /** Set the item from which to get the tip information
      @param item the item from which to get the tip information
      @param rect the rectangle around which the tip will be shown
      @param pixmap the pixmap to be shown. If 0, no pixmap is shown
      */
    void setItem( KFileItem *item, const QRect &rect = QRect(),
                  const QPixmap *pixmap = 0 );

    virtual bool eventFilter( QObject *, QEvent *e );

  protected:
    virtual void drawContents( QPainter *p );
    virtual void resizeEvent( QResizeEvent * );

  private Q_SLOTS:
    void gotPreview( const KFileItem*, const QPixmap& );
    void gotPreviewResult();

    void startDelayed();
    void showTip();
    void hideTip();

  private:
    void setFilter( bool enable );

    void reposition();

    QLabel*    m_iconLabel;
    QLabel*    m_textLabel;
    bool       m_on : 1;
    bool       m_preview : 1;  // shall the preview icon be shown
    bool       m_filter : 1;
    QPixmap    m_corners[4];
    int        m_corner;
    int        m_num;
    Q3ScrollView* m_view;
    KFileItem* m_item;
    KIO::PreviewJob* m_previewJob;
    QRect      m_rect;
    QTimer*    m_timer;
};

#endif
