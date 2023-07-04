/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfileitemclipboard.h"

#include <KUrlMimeData>

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

class KFileItemClipboardSingleton
{
public:
    KFileItemClipboard instance;
};
Q_GLOBAL_STATIC(KFileItemClipboardSingleton, s_KFileItemClipboard)

KFileItemClipboard *KFileItemClipboard::instance()
{
    return &s_KFileItemClipboard->instance;
}

bool KFileItemClipboard::isCut(const QUrl &url) const
{
    return m_cutItems.contains(url);
}

QList<QUrl> KFileItemClipboard::cutItems() const
{
    return m_cutItems.values();
}

KFileItemClipboard::~KFileItemClipboard()
{
}

void KFileItemClipboard::updateCutItems()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();

    // mimeData can be 0 according to https://bugs.kde.org/show_bug.cgi?id=335053
    if (!mimeData) {
        m_cutItems.clear();
        Q_EMIT cutItemsChanged();
        return;
    }

    const QByteArray data = mimeData->data(QStringLiteral("application/x-kde-cutselection"));
    const bool isCutSelection = (!data.isEmpty() && data.at(0) == QLatin1Char('1'));
    if (isCutSelection) {
        const auto urlsFromMimeData = KUrlMimeData::urlsFromMimeData(mimeData);
        m_cutItems = QSet<QUrl>(urlsFromMimeData.constBegin(), urlsFromMimeData.constEnd());
    } else {
        m_cutItems.clear();
    }
    Q_EMIT cutItemsChanged();
}

KFileItemClipboard::KFileItemClipboard()
    : QObject(nullptr)
    , m_cutItems()
{
    updateCutItems();

    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &KFileItemClipboard::updateCutItems);
}

#include "moc_kfileitemclipboard.cpp"
