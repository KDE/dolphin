/*
 * SPDX-FileCopyrightText: 2025 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class SetFolderIconItemAction : public KAbstractFileItemActionPlugin
{
    Q_OBJECT

public:
    SetFolderIconItemAction(QObject *parent);

    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override;

private:
    void setFolderIcon(bool check);
    QUrl m_url;
    QUrl m_localUrl;
};
