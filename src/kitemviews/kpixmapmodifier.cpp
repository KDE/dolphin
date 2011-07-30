//krazy:exclude=copyright (email of Maxim is missing)
/*
    This file is a part of the KDE project

    Copyright © 2006 Zack Rusin <zack@kde.org>
    Copyright © 2006-2007, 2008 Fredrik Höglund <fredrik@kde.org>

    The stack blur algorithm was invented by Mario Klingemann <mario@quasimondo.com>

    This implementation is based on the version in Anti-Grain Geometry Version 2.4,
    Copyright © 2002-2005 Maxim Shemanarev (http://www.antigrain.com)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "kpixmapmodifier_p.h"

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QSize>

#include <KDebug>

#include <config-X11.h> // for HAVE_XRENDER
#if defined(Q_WS_X11) && defined(HAVE_XRENDER)
#  include <QX11Info>
#  include <X11/Xlib.h>
#  include <X11/extensions/Xrender.h>
#endif

static const quint32 stackBlur8Mul[255] =
{
    512,512,456,512,328,456,335,512,405,328,271,456,388,335,292,512,
    454,405,364,328,298,271,496,456,420,388,360,335,312,292,273,512,
    482,454,428,405,383,364,345,328,312,298,284,271,259,496,475,456,
    437,420,404,388,374,360,347,335,323,312,302,292,282,273,265,512,
    497,482,468,454,441,428,417,405,394,383,373,364,354,345,337,328,
    320,312,305,298,291,284,278,271,265,259,507,496,485,475,465,456,
    446,437,428,420,412,404,396,388,381,374,367,360,354,347,341,335,
    329,323,318,312,307,302,297,292,287,282,278,273,269,265,261,512,
    505,497,489,482,475,468,461,454,447,441,435,428,422,417,411,405,
    399,394,389,383,378,373,368,364,359,354,350,345,341,337,332,328,
    324,320,316,312,309,305,301,298,294,291,287,284,281,278,274,271,
    268,265,262,259,257,507,501,496,491,485,480,475,470,465,460,456,
    451,446,442,437,433,428,424,420,416,412,408,404,400,396,392,388,
    385,381,377,374,370,367,363,360,357,354,350,347,344,341,338,335,
    332,329,326,323,320,318,315,312,310,307,304,302,299,297,294,292,
    289,287,285,282,280,278,275,273,271,269,267,265,263,261,259
};

static const quint32 stackBlur8Shr[255] =
{
    9, 11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17,
    17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24
};

static void blurHorizontal(QImage& image, unsigned int* stack, int div, int radius)
{
    int stackindex;
    int stackstart;

    quint32 * const pixels = reinterpret_cast<quint32 *>(image.bits());
    quint32 pixel;

    int w = image.width();
    int h = image.height();
    int wm = w - 1;

    unsigned int mulSum = stackBlur8Mul[radius];
    unsigned int shrSum = stackBlur8Shr[radius];

    unsigned int sum, sumIn, sumOut;

    for (int y = 0; y < h; y++) {
        sum    = 0;
        sumIn  = 0;
        sumOut = 0;

        const int yw = y * w;
        pixel = pixels[yw];
        for (int i = 0; i <= radius; i++) {
            stack[i] = qAlpha(pixel);

            sum += stack[i] * (i + 1);
            sumOut += stack[i];
        }

        for (int i = 1; i <= radius; i++) {
            pixel = pixels[yw + qMin(i, wm)];

            unsigned int* stackpix = &stack[i + radius];
            *stackpix = qAlpha(pixel);

            sum    += *stackpix * (radius + 1 - i);
            sumIn += *stackpix;
        }

        stackindex = radius;
        for (int x = 0, i = yw; x < w; x++) {
            pixels[i++] = (((sum * mulSum) >> shrSum) << 24) & 0xff000000;

            sum -= sumOut;

            stackstart = stackindex + div - radius;
            if (stackstart >= div) {
                stackstart -= div;
            }

            unsigned int* stackpix = &stack[stackstart];

            sumOut -= *stackpix;

            pixel = pixels[yw + qMin(x + radius + 1, wm)];

            *stackpix = qAlpha(pixel);

            sumIn += *stackpix;
            sum    += sumIn;

            if (++stackindex >= div) {
                stackindex = 0;
            }

            stackpix = &stack[stackindex];

            sumOut += *stackpix;
            sumIn  -= *stackpix;
        }
    }
}

static void blurVertical(QImage& image, unsigned int* stack, int div, int radius)
{
    int stackindex;
    int stackstart;

    quint32 * const pixels = reinterpret_cast<quint32 *>(image.bits());
    quint32 pixel;

    int w = image.width();
    int h = image.height();
    int hm = h - 1;

    int mul_sum = stackBlur8Mul[radius];
    int shr_sum = stackBlur8Shr[radius];

    unsigned int sum, sumIn, sumOut;

    for (int x = 0; x < w; x++) {
        sum    = 0;
        sumIn  = 0;
        sumOut = 0;

        pixel = pixels[x];
        for (int i = 0; i <= radius; i++) {
            stack[i] = qAlpha(pixel);

            sum += stack[i] * (i + 1);
            sumOut += stack[i];
        }

        for (int i = 1; i <= radius; i++) {
            pixel = pixels[qMin(i, hm) * w + x];

            unsigned int* stackpix = &stack[i + radius];
            *stackpix = qAlpha(pixel);

            sum    += *stackpix * (radius + 1 - i);
            sumIn += *stackpix;
        }

        stackindex = radius;
        for (int y = 0, i = x; y < h; y++, i += w) {
            pixels[i] = (((sum * mul_sum) >> shr_sum) << 24) & 0xff000000;

            sum -= sumOut;

            stackstart = stackindex + div - radius;
            if (stackstart >= div)
                stackstart -= div;

            unsigned int* stackpix = &stack[stackstart];

            sumOut -= *stackpix;

            pixel = pixels[qMin(y + radius + 1, hm) * w + x];

            *stackpix = qAlpha(pixel);

            sumIn += *stackpix;
            sum    += sumIn;

            if (++stackindex >= div) {
                stackindex = 0;
            }

            stackpix = &stack[stackindex];

            sumOut += *stackpix;
            sumIn  -= *stackpix;
        }
    }
}

static void stackBlur(QImage& image, float radius)
{
    radius = qRound(radius);

    int div = int(radius * 2) + 1;
    unsigned int* stack  = new unsigned int[div];

    blurHorizontal(image, stack, div, radius);
    blurVertical(image, stack, div, radius);

    delete [] stack;
}

static void shadowBlur(QImage& image, float radius, const QColor& color)
{
    if (radius < 0) {
        return;
    }

    if (radius > 0) {
        stackBlur(image, radius);
    }

    // Correct the color and opacity of the shadow
    QPainter p(&image);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(image.rect(), color);
}

namespace {
    /** Helper class for drawing frames for KPixmapModifier::applyFrame(). */
    class TileSet
    {
    public:
        enum { LeftMargin = 3, TopMargin = 2, RightMargin = 3, BottomMargin = 4 };

        enum Tile { TopLeftCorner = 0, TopSide, TopRightCorner, LeftSide,
                    RightSide, BottomLeftCorner, BottomSide, BottomRightCorner,
                    NumTiles };

        TileSet()
        {
            QImage image(8 * 3, 8 * 3, QImage::Format_ARGB32_Premultiplied);

            QPainter p(&image);
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.fillRect(image.rect(), Qt::transparent);
            p.fillRect(image.rect().adjusted(3, 3, -3, -3), Qt::black);
            p.end();

            shadowBlur(image, 3, Qt::black);

            QPixmap pixmap = QPixmap::fromImage(image);
            m_tiles[TopLeftCorner]     = pixmap.copy(0, 0, 8, 8);
            m_tiles[TopSide]           = pixmap.copy(8, 0, 8, 8);
            m_tiles[TopRightCorner]    = pixmap.copy(16, 0, 8, 8);
            m_tiles[LeftSide]          = pixmap.copy(0, 8, 8, 8);
            m_tiles[RightSide]         = pixmap.copy(16, 8, 8, 8);
            m_tiles[BottomLeftCorner]  = pixmap.copy(0, 16, 8, 8);
            m_tiles[BottomSide]        = pixmap.copy(8, 16, 8, 8);
            m_tiles[BottomRightCorner] = pixmap.copy(16, 16, 8, 8);
        }

        void paint(QPainter* p, const QRect& r)
        {
            p->drawPixmap(r.topLeft(), m_tiles[TopLeftCorner]);
            if (r.width() - 16 > 0) {
                p->drawTiledPixmap(r.x() + 8, r.y(), r.width() - 16, 8, m_tiles[TopSide]);
            }
            p->drawPixmap(r.right() - 8 + 1, r.y(), m_tiles[TopRightCorner]);
            if (r.height() - 16 > 0) {
                p->drawTiledPixmap(r.x(), r.y() + 8, 8, r.height() - 16,  m_tiles[LeftSide]);
                p->drawTiledPixmap(r.right() - 8 + 1, r.y() + 8, 8, r.height() - 16, m_tiles[RightSide]);
            }
            p->drawPixmap(r.x(), r.bottom() - 8 + 1, m_tiles[BottomLeftCorner]);
            if (r.width() - 16 > 0) {
                p->drawTiledPixmap(r.x() + 8, r.bottom() - 8 + 1, r.width() - 16, 8, m_tiles[BottomSide]);
            }
            p->drawPixmap(r.right() - 8 + 1, r.bottom() - 8 + 1, m_tiles[BottomRightCorner]);

            const QRect contentRect = r.adjusted(LeftMargin + 1, TopMargin + 1,
                                                 -(RightMargin + 1), -(BottomMargin + 1));
            p->fillRect(contentRect, Qt::transparent);
        }

        QPixmap m_tiles[NumTiles];
    };
}

void KPixmapModifier::scale(QPixmap& pixmap, const QSize& scaledSize)
{
#if defined(Q_WS_X11) && defined(HAVE_XRENDER)
    // Assume that the texture size limit is 2048x2048
    if ((pixmap.width() <= 2048) && (pixmap.height() <= 2048) && pixmap.x11PictureHandle()) {
        QSize scaledPixmapSize = pixmap.size();
        scaledPixmapSize.scale(scaledSize, Qt::KeepAspectRatio);

        const qreal factor = scaledPixmapSize.width() / qreal(pixmap.width());

        XTransform xform = {{
            { XDoubleToFixed(1 / factor), 0, 0 },
            { 0, XDoubleToFixed(1 / factor), 0 },
            { 0, 0, XDoubleToFixed(1) }
        }};

        QPixmap scaledPixmap(scaledPixmapSize);
        scaledPixmap.fill(Qt::transparent);

        Display* dpy = QX11Info::display();

        XRenderPictureAttributes attr;
        attr.repeat = RepeatPad;
        XRenderChangePicture(dpy, pixmap.x11PictureHandle(), CPRepeat, &attr);

        XRenderSetPictureFilter(dpy, pixmap.x11PictureHandle(), FilterBilinear, 0, 0);
        XRenderSetPictureTransform(dpy, pixmap.x11PictureHandle(), &xform);
        XRenderComposite(dpy, PictOpOver, pixmap.x11PictureHandle(), None, scaledPixmap.x11PictureHandle(),
                         0, 0, 0, 0, 0, 0, scaledPixmap.width(), scaledPixmap.height());
        pixmap = scaledPixmap;
    } else {
        pixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
#else
    pixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::FastTransformation);
#endif
}

void KPixmapModifier::applyFrame(QPixmap& icon, const QSize& scaledSize)
{
    static TileSet tileSet;

    // Resize the icon to the maximum size minus the space required for the frame
    const QSize size(scaledSize.width() - TileSet::LeftMargin - TileSet::RightMargin,
                     scaledSize.height() - TileSet::TopMargin - TileSet::BottomMargin);
    scale(icon, size);

    QPixmap framedIcon(icon.size().width() + TileSet::LeftMargin + TileSet::RightMargin,
                       icon.size().height() + TileSet::TopMargin + TileSet::BottomMargin);
    framedIcon.fill(Qt::transparent);

    QPainter painter;
    painter.begin(&framedIcon);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    tileSet.paint(&painter, framedIcon.rect());
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawPixmap(TileSet::LeftMargin, TileSet::TopMargin, icon);

    icon = framedIcon;
}

