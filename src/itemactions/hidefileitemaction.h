/*
 * SPDX-FileCopyrightText: 2025 Kostiantyn Korchuhanov <kostiantyn.korchanov@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HIDEFILEITEMACTION_H
#define HIDEFILEITEMACTION_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>
#include <QAction>
#include <QList>
#include <QWidget>

class HideFileItemAction : public KAbstractFileItemActionPlugin
{
    Q_OBJECT

public:
    HideFileItemAction(QObject *parent);

    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override;
};

#endif // HIDEFILEITEMACTION_H
