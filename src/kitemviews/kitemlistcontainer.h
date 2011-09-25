/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#ifndef KITEMLISTCONTAINER_H
#define KITEMLISTCONTAINER_H

#include <libdolphin_export.h>

#include <QAbstractAnimation>
#include <QAbstractScrollArea>

class KItemListController;
class KItemListView;
class KItemModelBase;
class QPropertyAnimation;

/**
 * @brief Provides a QWidget based scrolling view for a KItemListController.
 *
 * @see KItemListController
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListContainer : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit KItemListContainer(KItemListController* controller, QWidget* parent = 0);
    KItemListContainer(QWidget* parent = 0);
    virtual ~KItemListContainer();

    KItemListController* controller() const;

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void scrollContentsBy(int dx, int dy);
    virtual bool eventFilter(QObject* obj, QEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

private slots:
    void slotModelChanged(KItemModelBase* current, KItemModelBase* previous);
    void slotViewChanged(KItemListView* current, KItemListView* previous);
    void slotAnimationStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    void scrollTo(qreal offset);
    void updateScrollOffsetScrollBar();
    void updateItemOffsetScrollBar();

private:
    void initialize();
    void updateGeometries();

private:
    KItemListController* m_controller;

    bool m_scrollBarPressed;
    bool m_smoothScrolling;
    QPropertyAnimation* m_smoothScrollingAnimation;
};

#endif


