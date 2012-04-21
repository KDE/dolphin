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

#include "kfileitemlistwidget.h"

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KIO/MetaData>
#include <QDateTime>

KFileItemListWidgetInformant::KFileItemListWidgetInformant() :
    KStandardItemListWidgetInformant()
{
}

KFileItemListWidgetInformant::~KFileItemListWidgetInformant()
{
}

QString KFileItemListWidgetInformant::roleText(const QByteArray& role,
                                               const QHash<QByteArray, QVariant>& values) const
{
    QString text;
    const QVariant roleValue = values.value(role);

    // Implementation note: In case if more roles require a custom handling
    // use a hash + switch for a linear runtime.

    if (role == "size") {
        if (values.value("isDir").toBool()) {
            // The item represents a directory. Show the number of sub directories
            // instead of the file size of the directory.
            if (!roleValue.isNull()) {
                const int count = roleValue.toInt();
                if (count < 0) {
                    text = i18nc("@item:intable", "Unknown");
                } else {
                    text = i18ncp("@item:intable", "%1 item", "%1 items", count);
                }
            }
        } else {
            const KIO::filesize_t size = roleValue.value<KIO::filesize_t>();
            text = KGlobal::locale()->formatByteSize(size);
        }
    } else if (role == "date") {
        const QDateTime dateTime = roleValue.toDateTime();
        text = KGlobal::locale()->formatDateTime(dateTime);
    } else {
        text = KStandardItemListWidgetInformant::roleText(role, values);
    }

    return text;
}

KFileItemListWidget::KFileItemListWidget(KItemListWidgetInformant* informant, QGraphicsItem* parent) :
    KStandardItemListWidget(informant, parent)
{
}

KFileItemListWidget::~KFileItemListWidget()
{
}

KItemListWidgetInformant* KFileItemListWidget::createInformant()
{
    return new KFileItemListWidgetInformant();
}

bool KFileItemListWidget::isRoleRightAligned(const QByteArray& role) const
{
    return role == "size";
}

#include "kfileitemlistwidget.moc"
