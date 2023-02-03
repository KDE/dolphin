/*
 * SPDX-FileCopyrightText: 2017 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "dolphin_export.h"

#include <QObject>
#include <QPointer>

class QAction;

/**
 * An event filter that allows to detect a middle click
 * to trigger e.g. opening something in a new tab.
 */
class DOLPHIN_EXPORT MiddleClickActionEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit MiddleClickActionEventFilter(QObject *parent);
    ~MiddleClickActionEventFilter() override;

Q_SIGNALS:
    void actionMiddleClicked(QAction *action);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPointer<QAction> m_lastMiddlePressedAction;
};
