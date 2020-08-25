/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTCONTAINER_H
#define KITEMLISTCONTAINER_H

#include "dolphin_export.h"

#include <QAbstractScrollArea>

class KItemListController;
class KItemListSmoothScroller;
class KItemListView;
class KItemModelBase;

/**
 * @brief Provides a QWidget based scrolling view for a KItemListController.
 *
 * The model and view are maintained by the KItemListController.
 *
 * @see KItemListController
 */
class DOLPHIN_EXPORT KItemListContainer : public QAbstractScrollArea
{
    Q_OBJECT

public:
    /**
     * @param controller Controller that maintains the model and the view.
     *                   The KItemListContainer takes ownership of the controller
     *                   (the parent will be set to the KItemListContainer).
     * @param parent     Optional parent widget.
     */
    explicit KItemListContainer(KItemListController* controller, QWidget* parent = nullptr);
    ~KItemListContainer() override;
    KItemListController* controller() const;

    void setEnabledFrame(bool enable);
    bool enabledFrame() const;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void slotScrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);
    void slotModelChanged(KItemModelBase* current, KItemModelBase* previous);
    void slotViewChanged(KItemListView* current, KItemListView* previous);
    void scrollTo(qreal offset);
    void updateScrollOffsetScrollBar();
    void updateItemOffsetScrollBar();

private:
    void updateGeometries();
    void updateSmoothScrollers(Qt::Orientation orientation);

    /**
     * Helper method for updateScrollOffsetScrollBar(). Updates the scrollbar-policy
     * to Qt::ScrollBarAlwaysOn for cases where turning off the scrollbar might lead
     * to an endless layout loop (see bug #293318).
     */
    void updateScrollOffsetScrollBarPolicy();

private:
    KItemListController* m_controller;

    KItemListSmoothScroller* m_horizontalSmoothScroller;
    KItemListSmoothScroller* m_verticalSmoothScroller;
};

#endif


