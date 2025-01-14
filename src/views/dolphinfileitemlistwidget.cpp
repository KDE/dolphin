/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinfileitemlistwidget.h"
#include "../kitemviews/private/kitemviewsutils.h"

#include "dolphindebug.h"

#include <KIconLoader>

DolphinFileItemListWidget::DolphinFileItemListWidget(KItemListWidgetInformant *informant, QGraphicsItem *parent)
    : KFileItemListWidget(informant, parent)
{
}

DolphinFileItemListWidget::~DolphinFileItemListWidget()
{
}

void DolphinFileItemListWidget::refreshCache()
{
    QColor color;
    const QHash<QByteArray, QVariant> values = data();
    QHash<Qt::Corner, QString> overlays;
    if (values.contains("version")) {
        // The item is under version control. Apply the text color corresponding
        // to its version state.
        const KVersionControlPlugin::ItemVersion version = static_cast<KVersionControlPlugin::ItemVersion>(values.value("version").toInt());
        const QColor textColor = styleOption().palette.text().color();
        QColor tintColor = textColor;

        // Using hardcoded colors is generally a bad idea. In this case the colors just act
        // as tint colors and are mixed with the current set text color. The tint colors
        // have been optimized for the base colors of the corresponding Oxygen emblems.
        switch (version) {
        case KVersionControlPlugin::UpdateRequiredVersion:
            tintColor = Qt::yellow;
            break;
        case KVersionControlPlugin::LocallyModifiedUnstagedVersion:
            tintColor = Qt::green;
            break;
        case KVersionControlPlugin::LocallyModifiedVersion:
            tintColor = Qt::green;
            break;
        case KVersionControlPlugin::AddedVersion:
            tintColor = Qt::green;
            break;
        case KVersionControlPlugin::RemovedVersion:
            tintColor = Qt::darkRed;
            break;
        case KVersionControlPlugin::ConflictingVersion:
            tintColor = Qt::red;
            break;
        case KVersionControlPlugin::IgnoredVersion:
            tintColor = Qt::white;
            break;
        case KVersionControlPlugin::MissingVersion:
            tintColor = Qt::red;
            break;
        case KVersionControlPlugin::NormalVersion:
        case KVersionControlPlugin::UnversionedVersion:
        default:
            break;
        }

        color = QColor((tintColor.red() + textColor.red()) / 2,
                       (tintColor.green() + textColor.green()) / 2,
                       (tintColor.blue() + textColor.blue()) / 2,
                       (tintColor.alpha() + textColor.alpha()) / 2);

        overlays.insert(Qt::Corner::BottomLeftCorner, overlayForState(version));
    }

    if (values.contains("iconOverlays")) {
        const auto corners = {Qt::Corner::BottomRightCorner, Qt::Corner::TopLeftCorner, Qt::Corner::TopRightCorner};
        const auto iconOverlays = values.value("iconOverlays").toStringList();
        auto overlaysIt = iconOverlays.constBegin();
        for (const auto &corner : corners) {
            if (overlaysIt == iconOverlays.constEnd()) {
                break;
            }
            overlays.insert(corner, *overlaysIt);
            overlaysIt = ++overlaysIt;
        }
    }

    setOverlays(overlays);
    setTextColor(color);
}

QString DolphinFileItemListWidget::overlayForState(KVersionControlPlugin::ItemVersion version) const
{
    QString iconName;
    switch (version) {
    case KVersionControlPlugin::NormalVersion:
        iconName = QStringLiteral("vcs-normal");
        break;
    case KVersionControlPlugin::UpdateRequiredVersion:
        iconName = QStringLiteral("vcs-update-required");
        break;
    case KVersionControlPlugin::LocallyModifiedVersion:
        iconName = QStringLiteral("vcs-locally-modified");
        break;
    case KVersionControlPlugin::LocallyModifiedUnstagedVersion:
        iconName = QStringLiteral("vcs-locally-modified-unstaged");
        break;
    case KVersionControlPlugin::AddedVersion:
        iconName = QStringLiteral("vcs-added");
        break;
    case KVersionControlPlugin::RemovedVersion:
        iconName = QStringLiteral("vcs-removed");
        break;
    case KVersionControlPlugin::ConflictingVersion:
        iconName = QStringLiteral("vcs-conflicting");
        break;
    case KVersionControlPlugin::UnversionedVersion:
    case KVersionControlPlugin::IgnoredVersion:
    case KVersionControlPlugin::MissingVersion:
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    return iconName;
}

#include "moc_dolphinfileitemlistwidget.cpp"
