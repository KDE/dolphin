/***************************************************************************
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef KTOOLTIP_H
#define KTOOLTIP_H

#include <QObject>
#include <QPalette>
#include <QFont>
#include <QRect>
#include <QStyle>
#include <QFontMetrics>

class QString;
class QIcon;
class QSize;
class QPainter;
class QRegion;

class KToolTipItemPrivate;

/**
 * KToolTipItem contains the data to be displayed in a tooltip.
 *
 * Custom data can be stored as QVariants in the object by calling
 * setData() with a custom item role, and retrieved and displayed
 * by a tooltip delegate by calling data().
 *
 * The default tooltip delegate uses Qt::DecorationRole and
 * Qt::DisplayRole.
 *
 * To display the tooltip, call KToolTip::showTip() with a pointer
 * to the KToolTipItem.
 *
 * You can reimplement the setData() and/or data() methods in this
 * class to implement on-demand loading of data.
 */
class KToolTipItem
{
public:
     enum ItemType { DefaultType, UserType = 1000 };

     /**
      * Creates a KToolTipItem with @p text and no icon.
      */
     KToolTipItem(const QString &text, int type = DefaultType);

     /**
      * Creates a KToolTipItem with an @p icon and @p text.
      */
     KToolTipItem(const QIcon &icon, const QString &text, int type = DefaultType);

     /**
      * Destroys the KToolTipItem.
      */
     virtual ~KToolTipItem();

     /**
      * Returns the item type.
      */
     int type() const;

     QString text() const;
     QIcon icon() const;

     virtual QVariant data(int role) const;
     virtual void setData(int role, const QVariant &data);

private:
     KToolTipItemPrivate * const d;
};


class KStyleOptionToolTip
{
public:
    KStyleOptionToolTip();
    enum Corner { TopLeftCorner, TopRightCorner, BottomLeftCorner, BottomRightCorner, NoCorner };

    Qt::LayoutDirection direction;
    QFontMetrics fontMetrics;
    QPalette palette;
    QRect rect;
    QStyle::State state;
    QFont font;
    QSize decorationSize;
    Corner activeCorner;
};

/**
 * KToolTipDelegate is responsible for providing the size hint and
 * painting the tooltips.
 */
class KToolTipDelegate : public QObject
{
public:
     KToolTipDelegate();
     virtual ~KToolTipDelegate();

     virtual QSize sizeHint(const KStyleOptionToolTip *option, const KToolTipItem *item) const;

     /**
      * If haveAlphaChannel() returns true, the paint device will be filled with
      * Qt::transparent when this function is called, otherwise the content is
      * undefined.
      */
     virtual void paint(QPainter *painter, const KStyleOptionToolTip *option,
                        const KToolTipItem *item) const;

     /**
      * Reimplement this function to specify the region of the tooltip
      * that accepts input. Any mouse events that occur outside this
      * region will be sent to the widget below the tooltip.
      *
      * The default implemenation returns a region containing the
      * bounding rect of the tooltip.
      *
      * This function will only be called if haveAlphaChannel()
      * returns true.
      */
     virtual QRegion inputShape(const KStyleOptionToolTip *option) const;

     /**
      * Reimplement this function to specify a shape mask for the tooltip.
      *
      * The default implemenation returns a region containing the
      * bounding rect of the tooltip.
      *
      * This function will only be called if haveAlphaChannel()
      * returns false.
      */
     virtual QRegion shapeMask(const KStyleOptionToolTip *option) const;

protected:
     /**
      * Returns true if the tooltip has an alpha channel, and false
      * otherwise.
      *
      * Implementors should assume that this condition may change at
      * any time during the runtime of the application, as compositing
      * can be enabled or disabled in the window manager.
      */
     bool haveAlphaChannel() const;

#if 0
private Q_SLOTS:
     /**
      * Schedules a repaint of the tooltip item.
      * This slot can be connected to a timer to animate the tooltip.
      */
     void update(const KToolTipItem *item);
#endif
};


/**
 * KToolTip provides customizable tooltips that can have animations as well as an alpha
 * channel, allowing for dynamic transparency effects.
 *
 * ARGB tooltips work on X11 even when the application isn't using the ARGB visual.
 */
namespace KToolTip
{
    void showText(const QPoint &pos, const QString &text, QWidget *widget, const QRect &rect);
    void showText(const QPoint &pos, const QString &text, QWidget *widget = 0);

    /**
     * Shows the tip @p item at the global position indicated by @p pos.
     *
     * Ownership of the item is transferred to KToolTip. The item will be deleted
     * automatically when it is hidden.
     *
     * The tip is shown immediately when this function is called.
     */
    void showTip(const QPoint &pos, KToolTipItem *item);
    void hideTip();

    void setToolTipDelegate(KToolTipDelegate *delegate);
}

#endif
