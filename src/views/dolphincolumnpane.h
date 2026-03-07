/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINCOLUMNPANE_H
#define DOLPHINCOLUMNPANE_H

#include "dolphin_export.h"

#include <KFileItem>
#include <QUrl>
#include <QWidget>

class DolphinItemListView;
class KFileItemModel;
class KItemListContainer;
class KItemListController;
class VersionControlObserver;

/**
 * @short A single column in the Miller Columns view.
 *
 * Each DolphinColumnPane displays the contents of one directory,
 * using the same kitemviews stack that Dolphin uses for its other
 * view modes (KFileItemModel + DolphinItemListView + KItemListContainer).
 */
class DOLPHIN_EXPORT DolphinColumnPane : public QWidget
{
    Q_OBJECT

public:
    /**
     * @param model Externally created model. Ownership is transferred to
     *              the internal KItemListController.
     */
    explicit DolphinColumnPane(KFileItemModel *model, QWidget *parent = nullptr);
    ~DolphinColumnPane() override;

    QUrl dirUrl() const;

    /**
     * Highlight the item that corresponds to @p childUrl
     * (i.e. mark the sub-folder that the next column represents).
     */
    void setActiveChildUrl(const QUrl &childUrl);
    void clearActiveChild();

    KFileItemModel *model() const;
    KItemListController *controller() const;
    KItemListContainer *container() const;

    void setPreviewsShown(bool show);

    int calculateOptimalWidth() const;

Q_SIGNALS:
    void directoryActivated(const QUrl &childDirUrl);
    void fileActivated(const KFileItem &item);
    void currentItemChanged(const KFileItem &item);
    void directoryLoadingCompleted();

    void itemContextMenuRequested(const KFileItem &item, const QPointF &screenPos);
    void viewContextMenuRequested(const QPointF &screenPos);

    void infoMessage(const QString &msg);
    void errorMessage(const QString &msg);
    void operationCompletedMessage(const QString &msg);

private Q_SLOTS:
    void slotItemActivated(int index);
    void slotCurrentChanged(int current, int previous);
    void slotItemContextMenuRequested(int index, const QPointF &pos);
    void slotViewContextMenuRequested(const QPointF &pos);

private:
    KFileItemModel *m_model = nullptr;
    DolphinItemListView *m_view = nullptr;
    KItemListController *m_controller = nullptr;
    KItemListContainer *m_container = nullptr;
    VersionControlObserver *m_versionControlObserver = nullptr;
};

#endif // DOLPHINCOLUMNPANE_H
