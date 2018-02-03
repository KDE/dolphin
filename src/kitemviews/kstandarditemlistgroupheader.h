/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KSTANDARDITEMLISTGROUPHEADER_H
#define KSTANDARDITEMLISTGROUPHEADER_H

#include "dolphin_export.h"

#include <kitemviews/kitemlistgroupheader.h>

#include <QPixmap>
#include <QStaticText>

class DOLPHIN_EXPORT KStandardItemListGroupHeader : public KItemListGroupHeader
{
    Q_OBJECT

public:
    explicit KStandardItemListGroupHeader(QGraphicsWidget* parent = nullptr);
    ~KStandardItemListGroupHeader() override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    void paintRole(QPainter* painter, const QRectF& roleBounds, const QColor& color) override;
    void paintSeparator(QPainter* painter, const QColor& color) override;
    void roleChanged(const QByteArray &current, const QByteArray &previous) override;
    void dataChanged(const QVariant& current, const QVariant& previous) override;
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;

private:
    void updateCache();

private:
    bool m_dirtyCache;
    QStaticText m_text;
    QPixmap m_pixmap;
};
#endif


