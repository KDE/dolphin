/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KFILEITEMCLIPBOARD_H
#define KFILEITEMCLIPBOARD_H

#include "dolphin_export.h"

#include <QList>
#include <QObject>
#include <QSet>
#include <QUrl>

/**
 * @brief Wrapper for QClipboard to provide fast access for checking
 *        whether a KFileItem has been clipped.
 */
class DOLPHIN_EXPORT KFileItemClipboard : public QObject
{
    Q_OBJECT

public:
    static KFileItemClipboard* instance();

    bool isCut(const QUrl& url) const;

    QList<QUrl> cutItems() const;

signals:
    void cutItemsChanged();

protected:
    ~KFileItemClipboard() override;

private slots:
    void updateCutItems();

private:
    KFileItemClipboard();

    QSet<QUrl> m_cutItems;

    friend class KFileItemClipboardSingleton;
};

#endif
