/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
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
 * @short One column of DolphinColumnsView, listing one folder.
 */
class DOLPHIN_EXPORT DolphinColumnPane : public QWidget
{
    Q_OBJECT

public:
    /// The controller of the pane takes the ownership of @p model.
    explicit DolphinColumnPane(KFileItemModel *model, QWidget *parent = nullptr);
    ~DolphinColumnPane() override;

    QUrl dirUrl() const;

    /// Marks the folder that the next column shows.
    void setActiveChildUrl(const QUrl &childUrl);
    /// Marks the child again, for a folder that had not listed when it was told.
    void reapplyActiveChildMark();

    /// A pending column has no width until its folder has listed.
    void setWidthPending(bool pending);
    bool isWidthPending() const;
    void clearActiveChild();

    KFileItemModel *model() const;
    KItemListController *controller() const;
    KItemListContainer *container() const;
    DolphinItemListView *itemListView() const;
    /// The item under the keyboard focus of this column, or a null item.
    KFileItem currentFileItem() const;
    /// The folder that @p item opens, a directory or an archive, or an empty url.
    static QUrl folderUrlFor(const KFileItem &item);

    void setPreviewsShown(bool show);

    int calculateOptimalWidth() const;

    void setZoomLevel(int level);

    void reloadSettings();

Q_SIGNALS:
    void directoryActivated(const QUrl &childDirUrl);
    void fileActivated(const KFileItem &item);
    void currentItemChanged(const KFileItem &item);
    void directoryLoadingCompleted();

    void infoMessage(const QString &msg);
    void errorMessage(const QString &msg);
    void operationCompletedMessage(const QString &msg);

private Q_SLOTS:
    void slotItemActivated(int index);
    void slotCurrentChanged(int current, int previous);

private:
    KFileItemModel *m_model = nullptr;
    DolphinItemListView *m_view = nullptr;
    KItemListController *m_controller = nullptr;
    KItemListContainer *m_container = nullptr;
    VersionControlObserver *m_versionControlObserver = nullptr;
    QUrl m_activeChildUrl;
    bool m_widthPending = false;
};

#endif // DOLPHINCOLUMNPANE_H
