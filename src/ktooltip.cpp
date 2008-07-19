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

#include "ktooltip.h"

#include <QApplication>
#include <QMap>
#include <QPixmap>
#include <QPainter>
#include <QVariant>
#include <QIcon>
#include <QWidget>
#include <QToolTip>
#include <QDebug>

#ifdef Q_WS_X11
#  include <QX11Info>
#  include <X11/Xlib.h>
#  include <X11/extensions/Xrender.h>
#  include <X11/extensions/shape.h>
#endif

#include "ktooltip_p.h"


// compile with XShape older than 1.0
#ifndef ShapeInput
const int ShapeInput = 2;
#endif


class KToolTipItemPrivate
{
public:
    QMap<int, QVariant> map;
    int type;
};

KToolTipItem::KToolTipItem(const QString &text, int type)
    : d(new KToolTipItemPrivate)
{
    d->map[Qt::DisplayRole] = text;
    d->type = type;
}

KToolTipItem::KToolTipItem(const QIcon &icon, const QString &text, int type)
    : d(new KToolTipItemPrivate)
{
    d->map[Qt::DecorationRole] = icon;
    d->map[Qt::DisplayRole]    = text;
    d->type = type;
}

KToolTipItem::~KToolTipItem()
{
    delete d;
}

int KToolTipItem::type() const
{
    return d->type;
}

QString KToolTipItem::text() const
{
    return data(Qt::DisplayRole).toString();
}

QIcon KToolTipItem::icon() const
{
    return qvariant_cast<QIcon>(data(Qt::DecorationRole));
}

QVariant KToolTipItem::data(int role) const
{
    return d->map.value(role);
}

void KToolTipItem::setData(int role, const QVariant &data)
{
    d->map[role] = data;
}



// ----------------------------------------------------------------------------


KStyleOptionToolTip::KStyleOptionToolTip()
    : fontMetrics(QApplication::font())
{
}


// ----------------------------------------------------------------------------



KToolTipDelegate::KToolTipDelegate() : QObject()
{
}

KToolTipDelegate::~KToolTipDelegate()
{
}

QSize KToolTipDelegate::sizeHint(const KStyleOptionToolTip *option, const KToolTipItem *item) const
{
    QSize size;
    size.rwidth() = option->fontMetrics.width(item->text());
    size.rheight() = option->fontMetrics.lineSpacing();

    QIcon icon = item->icon();
    if (!icon.isNull()) {
        const QSize iconSize = icon.actualSize(option->decorationSize);
        size.rwidth() += iconSize.width() + 4;
        size.rheight() = qMax(size.height(), iconSize.height());
    }

    return size + QSize(20, 20);

}

void KToolTipDelegate::paint(QPainter *painter, const KStyleOptionToolTip *option,
                             const KToolTipItem *item) const
{
    bool haveAlpha = haveAlphaChannel();
    painter->setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    if (haveAlpha)
        path.addRoundRect(option->rect.adjusted(0, 0, -1, -1), 25);
    else
        path.addRect(option->rect.adjusted(0, 0, -1, -1));

#if QT_VERSION >= 0x040400
    QColor color = option->palette.color(QPalette::ToolTipBase);
#else
    QColor color = option->palette.color(QPalette::Base);
#endif
    QColor from = color.lighter(105);
    QColor to   = color.darker(120);

    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0, from);
    gradient.setColorAt(1, to);

    painter->translate(.5, .5);
    painter->setPen(QPen(Qt::black, 1));
    painter->setBrush(gradient);
    painter->drawPath(path);
    painter->translate(-.5, -.5);

    if (haveAlpha) {
        QLinearGradient mask(0, 0, 0, 1);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setColorAt(0, QColor(0, 0, 0, 192));
        gradient.setColorAt(1, QColor(0, 0, 0, 72));
        painter->setCompositionMode(QPainter::CompositionMode_DestinationIn);
        painter->fillRect(option->rect, gradient);
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    QRect textRect = option->rect.adjusted(10, 10, -10, -10);

    QIcon icon = item->icon();
    if (!icon.isNull()) {
        const QSize iconSize = icon.actualSize(option->decorationSize);
        painter->drawPixmap(textRect.topLeft(), icon.pixmap(iconSize));
        textRect.adjust(iconSize.width() + 4, 0, 0, 0);
    }
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, item->text());
}

QRegion KToolTipDelegate::inputShape(const KStyleOptionToolTip *option) const
{
    return QRegion(option->rect);
}

QRegion KToolTipDelegate::shapeMask(const KStyleOptionToolTip *option) const
{
    return QRegion(option->rect);
}

bool KToolTipDelegate::haveAlphaChannel() const
{
    return KToolTipManager::instance()->haveAlphaChannel();
}



// ----------------------------------------------------------------------------



class KAbstractToolTipLabel
{
public:
    KAbstractToolTipLabel() {}
    virtual ~KAbstractToolTipLabel() {}

    virtual void showTip(const QPoint &pos, const KToolTipItem *item) = 0;
    virtual void moveTip(const QPoint &pos) = 0;
    virtual void hideTip() = 0;

protected:
    KStyleOptionToolTip styleOption() const;
    KToolTipDelegate *delegate() const;
};

KStyleOptionToolTip KAbstractToolTipLabel::styleOption() const
{
     KStyleOptionToolTip option;
     KToolTipManager::instance()->initStyleOption(&option);
     return option;
}

KToolTipDelegate *KAbstractToolTipLabel::delegate() const
{
    return KToolTipManager::instance()->delegate();
}


// ----------------------------------------------------------------------------



class QWidgetLabel : public QWidget, public KAbstractToolTipLabel
{
public:
    QWidgetLabel() : QWidget(0, Qt::ToolTip) {}
    void showTip(const QPoint &pos, const KToolTipItem *item);
    void moveTip(const QPoint &pos);
    void hideTip();

private:
    void paintEvent(QPaintEvent*);
    QSize sizeHint() const;

private:
    const KToolTipItem *currentItem;
};

void QWidgetLabel::showTip(const QPoint &pos, const KToolTipItem *item)
{
    currentItem = item;
    move(pos);
    show();
}

void QWidgetLabel::hideTip()
{
    hide();
    currentItem = 0;
}

void QWidgetLabel::moveTip(const QPoint &pos)
{
    move(pos);
}

void QWidgetLabel::paintEvent(QPaintEvent*)
{
    KStyleOptionToolTip option = styleOption();
    option.rect = rect();

    setMask(delegate()->shapeMask(&option));

    QPainter p(this);
    p.setFont(option.font);
    p.setPen(QPen(option.palette.brush(QPalette::Text), 0));
    delegate()->paint(&p, &option, currentItem);
}

QSize QWidgetLabel::sizeHint() const
{
    if (!currentItem)
        return QSize();

    KStyleOptionToolTip option = styleOption();
    return delegate()->sizeHint(&option, currentItem);
}



// ----------------------------------------------------------------------------



#ifdef Q_WS_X11

// X11 specific label that displays the tip in an ARGB window.
class ArgbLabel : public KAbstractToolTipLabel
{
public:
    ArgbLabel(Visual *visual, int depth);
    ~ArgbLabel();

    void showTip(const QPoint &pos, const KToolTipItem *item);
    void moveTip(const QPoint &pos);
    void hideTip();

private:
    Window    window;
    Colormap  colormap;
    Picture   picture;
    bool      mapped;
};

ArgbLabel::ArgbLabel(Visual *visual, int depth)
{
    Display *dpy = QX11Info::display();
    Window root = QX11Info::appRootWindow();
    colormap = XCreateColormap(dpy, QX11Info::appRootWindow(), visual, AllocNone);

    XSetWindowAttributes attr;
    attr.border_pixel      = 0;
    attr.background_pixel  = 0;
    attr.colormap          = colormap;
    attr.override_redirect = True;

    window = XCreateWindow(dpy, root, 0, 0, 1, 1, 0, depth, InputOutput, visual,
                           CWBorderPixel | CWBackPixel | CWColormap |
                           CWOverrideRedirect, &attr);

    // ### TODO: Set the WM hints so KWin can identify this window as a
    //           tooltip.

    XRenderPictFormat *format = XRenderFindVisualFormat(dpy, visual);
    picture = XRenderCreatePicture(dpy, window, format, 0, 0);

    mapped = false;
}

ArgbLabel::~ArgbLabel()
{
    Display *dpy = QX11Info::display();
    XRenderFreePicture(dpy, picture);
    XDestroyWindow(dpy, window);
    XFreeColormap(dpy, colormap);
}

void ArgbLabel::showTip(const QPoint &pos, const KToolTipItem *item)
{
    Display *dpy = QX11Info::display();
    KStyleOptionToolTip option = styleOption();
    const QSize size = delegate()->sizeHint(&option, item);
    option.rect = QRect(QPoint(), size);

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setFont(option.font);
    p.setPen(QPen(option.palette.brush(QPalette::Text), 0));
    delegate()->paint(&p, &option, item);

    // Resize, position and show the window.
    XMoveResizeWindow(dpy, window, pos.x(), pos.y(), size.width(), size.height());

    if (KToolTipManager::instance()->haveAlphaChannel()) {
        const QRegion region = delegate()->inputShape(&option);
        XShapeCombineRegion(dpy, window, ShapeInput, 0, 0, region.handle(), ShapeSet);
    } else {
        const QRegion region = delegate()->shapeMask(&option);
        XShapeCombineRegion(dpy, window, ShapeBounding, 0, 0, region.handle(), ShapeSet);
    }

    XMapWindow(dpy, window);

    // Blit the pixmap with the tip contents to the window.
    // Since the window is override-redirect and an ARGB32 window,
    // which always has an offscreen pixmap, there's no need to
    // wait for an Expose event, or to process those.
    XRenderComposite(dpy, PictOpSrc, pixmap.x11PictureHandle(), None,
                     picture, 0, 0, 0, 0, 0, 0, size.width(), size.height());

    mapped = true;
}

void ArgbLabel::moveTip(const QPoint &pos)
{
    if (mapped)
        XMoveWindow(QX11Info::display(), window, pos.x(), pos.y());
}

void ArgbLabel::hideTip()
{
    if (mapped) {
        Display *dpy = QX11Info::display();
        XUnmapWindow(dpy, window);
	mapped = false;
    }
}

#endif // Q_WS_X11




// ----------------------------------------------------------------------------




KToolTipManager *KToolTipManager::s_instance = 0;

KToolTipManager::KToolTipManager()
	: label(0), currentItem(0), m_delegate(0)
{
#ifdef Q_WS_X11
    Display *dpy      = QX11Info::display();
    int screen        = DefaultScreen(dpy);
    int depth         = DefaultDepth(dpy, screen);
    Visual *visual    = DefaultVisual(dpy, screen);
    net_wm_cm_s0      = XInternAtom(dpy, "_NET_WM_CM_S0", False);
    haveArgbVisual    = false;

    int nvi;
    XVisualInfo templ;
    templ.screen  = screen;
    templ.depth   = 32;
    templ.c_class = TrueColor;
    XVisualInfo *xvi = XGetVisualInfo(dpy, VisualScreenMask | VisualDepthMask |
                                      VisualClassMask, &templ, &nvi);

    for (int i = 0; i < nvi; ++i)
    {
        XRenderPictFormat *format = XRenderFindVisualFormat(dpy, xvi[i].visual);
        if (format->type == PictTypeDirect && format->direct.alphaMask)
        {
            visual   = xvi[i].visual;
            depth    = xvi[i].depth;
            haveArgbVisual = true;
            break;
        }
    }

    if (haveArgbVisual)
        label = new ArgbLabel(visual, depth);
    else
#endif
        label = new QWidgetLabel();
}

KToolTipManager::~KToolTipManager()
{
    delete label;
    delete currentItem;
}

void KToolTipManager::showTip(const QPoint &pos, KToolTipItem *item)
{
    hideTip();
    label->showTip(pos, item);
    currentItem = item;
}

void KToolTipManager::hideTip()
{
    label->hideTip();
    delete currentItem;
    currentItem = 0;
}

void KToolTipManager::initStyleOption(KStyleOptionToolTip *option) const
{
    option->direction      = QApplication::layoutDirection();
    option->fontMetrics    = QFontMetrics(QToolTip::font());
    option->activeCorner   = KStyleOptionToolTip::TopLeftCorner;
    option->palette        = QToolTip::palette();
    option->font           = QToolTip::font();
    option->rect           = QRect();
    option->state          = QStyle::State_None;
    option->decorationSize = QSize(32, 32);
}

bool KToolTipManager::haveAlphaChannel() const
{
#ifdef Q_WS_X11
    // ### This is a synchronous call - ideally we'd use a selection
    //     watcher to avoid it.
    return haveArgbVisual &&
        XGetSelectionOwner(QX11Info::display(), net_wm_cm_s0) != None;
#else
    return false;
#endif
}

void KToolTipManager::setDelegate(KToolTipDelegate *delegate)
{
    m_delegate = delegate;
}

KToolTipDelegate *KToolTipManager::delegate() const
{
    return m_delegate;
}



// ----------------------------------------------------------------------------



namespace KToolTip
{
    void showText(const QPoint &pos, const QString &text, QWidget *widget, const QRect &rect)
    {
        Q_UNUSED(widget)
        Q_UNUSED(rect)
        KToolTipItem *item = new KToolTipItem(text);
        KToolTipManager::instance()->showTip(pos, item);
    }

    void showText(const QPoint &pos, const QString &text, QWidget *widget)
    {
        showText(pos, text, widget, QRect());
    }

    void showTip(const QPoint &pos, KToolTipItem *item)
    {
        KToolTipManager::instance()->showTip(pos, item);
    }

    void hideTip()
    {
        KToolTipManager::instance()->hideTip();
    }

    void setToolTipDelegate(KToolTipDelegate *delegate)
    {
        KToolTipManager::instance()->setDelegate(delegate);
    }
}

