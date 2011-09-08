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

#include <KIcon>
#include <KIconLoader>
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
    QPixmap overlay;
    if (roles.contains("version")) {
        // The item is under version control. Apply the text color corresponding
        // to its version state.
        const KVersionControlPlugin::VersionState version = static_cast<KVersionControlPlugin::VersionState>(current.value("version").toInt());
        const QColor textColor = styleOption().palette.text().color();
        QColor tintColor = textColor;

        // Using hardcoded colors is generally a bad idea. In this case the colors just act
        // as tint colors and are mixed with the current set text color. The tint colors
        // have been optimized for the base colors of the corresponding Oxygen emblems.
        switch (version) {
        case KVersionControlPlugin::UpdateRequiredVersion:          tintColor = Qt::yellow; break;
        case KVersionControlPlugin::LocallyModifiedUnstagedVersion: tintColor = Qt::green; break;
        case KVersionControlPlugin::LocallyModifiedVersion:         tintColor = Qt::green; break;
        case KVersionControlPlugin::AddedVersion:                   tintColor = Qt::green; break;
        case KVersionControlPlugin::RemovedVersion:                 tintColor = Qt::darkRed; break;
        case KVersionControlPlugin::ConflictingVersion:             tintColor = Qt::red; break;
        case KVersionControlPlugin::UnversionedVersion:             tintColor = Qt::white; break;
        case KVersionControlPlugin::NormalVersion:
        default:
            break;
        }

        color = QColor((tintColor.red()   + textColor.red())   / 2,
                       (tintColor.green() + textColor.green()) / 2,
                       (tintColor.blue()  + textColor.blue())  / 2,
                       (tintColor.alpha() + textColor.alpha()) / 2);

        overlay = overlayForState(version, styleOption().iconSize);
    }

    setTextColor(color);
    setOverlay(overlay);
}

void DolphinFileItemListWidget::styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous)
{
    KFileItemListWidget::styleOptionChanged(current, previous);

    if (!overlay().isNull() && current.iconSize != previous.iconSize) {
        const KVersionControlPlugin::VersionState version = static_cast<KVersionControlPlugin::VersionState>(data().value("version").toInt());
        const QPixmap newOverlay = overlayForState(version, current.iconSize);
        setOverlay(newOverlay);
    }
}

QPixmap DolphinFileItemListWidget::overlayForState(KVersionControlPlugin::VersionState version, int size)
{
    int overlayHeight = KIconLoader::SizeSmall;
    if (size >= KIconLoader::SizeEnormous) {
        overlayHeight = KIconLoader::SizeMedium;
    } else if (size >= KIconLoader::SizeLarge) {
        overlayHeight = KIconLoader::SizeSmallMedium;
    } else if (size >= KIconLoader::SizeMedium) {
        overlayHeight = KIconLoader::SizeSmall;
    } else {
        overlayHeight = KIconLoader::SizeSmall / 2;
    }

    QString iconName;
    switch (version) {
    case KVersionControlPlugin::NormalVersion:
        iconName = "vcs-normal";
        break;
    case KVersionControlPlugin::UpdateRequiredVersion:
        iconName = "vcs-update-required";
        break;
    case KVersionControlPlugin::LocallyModifiedVersion:
        iconName = "vcs-locally-modified";
        break;
    case KVersionControlPlugin::LocallyModifiedUnstagedVersion:
        iconName = "vcs-locally-modified-unstaged";
        break;
    case KVersionControlPlugin::AddedVersion:
        iconName = "vcs-added";
        break;
    case KVersionControlPlugin::RemovedVersion:
        iconName = "vcs-removed";
        break;
    case KVersionControlPlugin::ConflictingVersion:
        iconName = "vcs-conflicting";
        break;
    case KVersionControlPlugin::UnversionedVersion:
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    return KIcon(iconName).pixmap(QSize(overlayHeight, overlayHeight));
}

#include "dolphinfileitemlistwidget.moc"
