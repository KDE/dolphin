/*
 * SPDX-FileCopyrightText: 2024 Ahmet Hakan Çelik <mail@ahakan.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MOVETONEWFOLDERITEMACTION_H
#define MOVETONEWFOLDERITEMACTION_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class QAction;
class QWidget;

class KNewFileMenu;

class MoveToNewFolderItemAction : public KAbstractFileItemActionPlugin
{
    Q_OBJECT

public:
    MoveToNewFolderItemAction(QObject *parent);

    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override;
};

#endif // MOVETONEWFOLDERITEMACTION_H
