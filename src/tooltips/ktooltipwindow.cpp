/*******************************************************************************
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>                   *
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                      *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#include "ktooltipwindow_p.h"

#include <kcolorscheme.h>

#include <QPainter>
#include <QVBoxLayout>

#ifdef Q_WS_X11
    #include <QX11Info>
#endif

KToolTipWindow::KToolTipWindow(QWidget* content) :
    QWidget(0)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(content);
}

KToolTipWindow::~KToolTipWindow()
{
}

void KToolTipWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    QColor toColor = palette().brush(QPalette::ToolTipBase).color();
    QColor fromColor = KColorScheme::shade(toColor, KColorScheme::LightShade, 0.2);

#ifdef Q_WS_X11
    const bool haveAlphaChannel = QX11Info::isCompositingManagerRunning();
#else
    const bool haveAlphaChannel = false;
#endif
    if (haveAlphaChannel) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(0.5, 0.5);
        toColor.setAlpha(220);
        fromColor.setAlpha(220);
    }

    QLinearGradient gradient(QPointF(0.0, 0.0), QPointF(width(), height()));
    gradient.setColorAt(0.0, fromColor);
    gradient.setColorAt(1.0, toColor);
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);

    const QRect rect(0, 0, width(), height());
    const qreal radius = 5;

    QPainterPath path;
    path.moveTo(rect.left(), rect.top() + radius);
    arc(path, rect.left()  + radius, rect.top()    + radius, radius, 180, -90);
    arc(path, rect.right() - radius, rect.top()    + radius, radius,  90, -90);
    arc(path, rect.right() - radius, rect.bottom() - radius, radius,   0, -90);
    arc(path, rect.left()  + radius, rect.bottom() - radius, radius, 270, -90);
    path.closeSubpath();

    painter.drawPath(path);
}

void KToolTipWindow::arc(QPainterPath& path, qreal cx, qreal cy, qreal radius, qreal angle, qreal sweeplength)
{
    path.arcTo(cx-radius, cy-radius, radius * 2, radius * 2, angle, sweeplength);
}

#include "ktooltipwindow_p.moc"
