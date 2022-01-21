/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KSTANDARDITEMLISTGROUPHEADER_H
#define KSTANDARDITEMLISTGROUPHEADER_H

#include "dolphin_export.h"
#include "kitemviews/kitemlistgroupheader.h"

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
    QString m_text;
    QPixmap m_pixmap;
};
#endif


