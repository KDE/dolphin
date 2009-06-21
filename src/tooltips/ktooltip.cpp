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
#include "ktooltip_p.h"
#include "ktooltipdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QWidget>
#include <QToolTip>

#ifdef Q_WS_X11
#  include <QX11Info>
#  include <X11/Xlib.h>
#  include <X11/extensions/shape.h>
#endif

// compile with XShape older than 1.0
#ifndef ShapeInput
const int ShapeInput = 2;
#endif


// ----------------------------------------------------------------------------


class KTipLabel : public QWidget
{
public:
    KTipLabel();
    void showTip(const QPoint &pos, const KToolTipItem *item);
    void moveTip(const QPoint &pos);
    void hideTip();

private:
    void paintEvent(QPaintEvent*);
    QSize sizeHint() const;
    KStyleOptionToolTip styleOption() const;
    KToolTipDelegate *delegate() const;

private:
    const KToolTipItem *m_currentItem;
};

KTipLabel::KTipLabel() : QWidget(0, Qt::ToolTip)
{
#ifdef Q_WS_X11
    if (QX11Info::isCompositingManagerRunning()) {
        setAttribute(Qt::WA_TranslucentBackground);
    }
#endif
}

void KTipLabel::showTip(const QPoint &pos, const KToolTipItem *item)
{
    m_currentItem = item;
    move(pos);
    show();
}

void KTipLabel::hideTip()
{
    hide();
    m_currentItem = 0;
}

void KTipLabel::moveTip(const QPoint &pos)
{
    move(pos);
}

void KTipLabel::paintEvent(QPaintEvent*)
{
    KStyleOptionToolTip option = styleOption();
    option.rect = rect();

#ifdef Q_WS_X11
    if (QX11Info::isCompositingManagerRunning())
        XShapeCombineRegion(x11Info().display(), winId(), ShapeInput, 0, 0,
                            delegate()->inputShape(option).handle(), ShapeSet);
    else
#endif
    setMask(delegate()->shapeMask(option));

    QPainter p(this);
    p.setFont(option.font);
    p.setPen(QPen(option.palette.brush(QPalette::Text), 0));
    delegate()->paint(&p, option, *m_currentItem);
}

QSize KTipLabel::sizeHint() const
{
    if (!m_currentItem)
        return QSize();

    KStyleOptionToolTip option = styleOption();
    return delegate()->sizeHint(option, *m_currentItem);
}

KStyleOptionToolTip KTipLabel::styleOption() const
{
     KStyleOptionToolTip option;
     KToolTipManager::instance()->initStyleOption(&option);
     return option;
}

KToolTipDelegate *KTipLabel::delegate() const
{
    return KToolTipManager::instance()->delegate();
}


// ----------------------------------------------------------------------------


KToolTipManager *KToolTipManager::s_instance = 0;

KToolTipManager::KToolTipManager()
    : m_label(new KTipLabel), m_currentItem(0), m_delegate(0)
{
}

KToolTipManager::~KToolTipManager()
{
    delete m_label;
    delete m_currentItem;
}

void KToolTipManager::showTip(const QPoint &pos, KToolTipItem *item)
{
    hideTip();
    m_label->showTip(pos, item);
    m_currentItem = item;
    m_tooltipPos = pos;
}

void KToolTipManager::hideTip()
{
    m_label->hideTip();
    delete m_currentItem;
    m_currentItem = 0;
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

void KToolTipManager::setDelegate(KToolTipDelegate *delegate)
{
    m_delegate = delegate;
}

void KToolTipManager::update()
{
    if (m_currentItem == 0)
        return;
    m_label->showTip(m_tooltipPos, m_currentItem);
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

