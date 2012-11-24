/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kfileitemclipboard.h"

#include <KGlobal>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>

class KFileItemClipboardSingleton
{
public:
    KFileItemClipboard instance;
};
K_GLOBAL_STATIC(KFileItemClipboardSingleton, s_KFileItemClipboard)



KFileItemClipboard* KFileItemClipboard::instance()
{
    return &s_KFileItemClipboard->instance;
}

bool KFileItemClipboard::isCut(const KUrl& url) const
{
    return m_cutItems.contains(url);
}

QList<KUrl> KFileItemClipboard::cutItems() const
{
    return m_cutItems.toList();
}

KFileItemClipboard::~KFileItemClipboard()
{
}

void KFileItemClipboard::updateCutItems()
{
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    m_cutItems = KUrl::List::fromMimeData(mimeData).toSet();
    emit cutItemsChanged();
}

KFileItemClipboard::KFileItemClipboard() :
    QObject(0),
    m_cutItems()
{
    updateCutItems();

    connect(QApplication::clipboard(), SIGNAL(dataChanged()),
            this, SLOT(updateCutItems()));
}

#include "kfileitemclipboard.moc"
