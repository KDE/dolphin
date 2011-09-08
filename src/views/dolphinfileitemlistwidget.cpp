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

#include "dolphinfileitemlistwidget.h"

#include <kversioncontrolplugin.h>
#include <QColor>

#include <KDebug>

DolphinFileItemListWidget::DolphinFileItemListWidget(QGraphicsItem* parent) :
    KFileItemListWidget(parent)
{
}

DolphinFileItemListWidget::~DolphinFileItemListWidget()
{
}

void DolphinFileItemListWidget::dataChanged(const QHash<QByteArray, QVariant>& current, const QSet<QByteArray>& roles)
{
    KFileItemListWidget::dataChanged(current, roles);

    QColor color;
    if (roles.contains("version")) {
        // The item is under version control. Apply the text color corresponding
        // to its version state.
        const KVersionControlPlugin::VersionState version = static_cast<KVersionControlPlugin::VersionState>(current.value("version").toInt());
        if (version != KVersionControlPlugin::UnversionedVersion) {
            const QColor textColor = styleOption().palette.text().color();
            QColor tintColor = textColor;

            // Using hardcoded colors is generally a bad idea. In this case the colors just act
            // as tint colors and are mixed with the current set text color. The tint colors
            // have been optimized for the base colors of the corresponding Oxygen emblems.
            switch (version) {
            case KVersionControlPlugin::UpdateRequiredVersion:          tintColor = Qt::yellow; break;
            case KVersionControlPlugin::LocallyModifiedUnstagedVersion: tintColor = Qt::darkGreen; break;
            case KVersionControlPlugin::LocallyModifiedVersion:         tintColor = Qt::green; break;
            case KVersionControlPlugin::AddedVersion:                   tintColor = Qt::green; break;
            case KVersionControlPlugin::RemovedVersion:                 tintColor = Qt::darkRed; break;
            case KVersionControlPlugin::ConflictingVersion:             tintColor = Qt::red; break;
            case KVersionControlPlugin::UnversionedVersion:
            case KVersionControlPlugin::NormalVersion:
            default:
                 // use the default text color
                 return;
            }

            color = QColor((tintColor.red()   + textColor.red())   / 2,
                           (tintColor.green() + textColor.green()) / 2,
                           (tintColor.blue()  + textColor.blue())  / 2,
                           (tintColor.alpha() + textColor.alpha()) / 2);
        }
    }

    setTextColor(color);
}

#include "dolphinfileitemlistwidget.moc"
