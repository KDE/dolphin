/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfileitemlistwidget.h"
#include "kfileitemlistview.h"
#include "kfileitemmodel.h"
#include "kitemlistview.h"

#include "dolphin_contentdisplaysettings.h"

#include <KFormat>
#include <KLocalizedString>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMimeDatabase>

KFileItemListWidgetInformant::KFileItemListWidgetInformant()
    : KStandardItemListWidgetInformant()
{
}

KFileItemListWidgetInformant::~KFileItemListWidgetInformant()
{
}

QString KFileItemListWidgetInformant::itemText(int index, const KItemListView *view) const
{
    Q_ASSERT(qobject_cast<KFileItemModel *>(view->model()));
    KFileItemModel *fileItemModel = static_cast<KFileItemModel *>(view->model());

    const KFileItem item = fileItemModel->fileItem(index);
    return item.text();
}

bool KFileItemListWidgetInformant::itemIsLink(int index, const KItemListView *view) const
{
    Q_ASSERT(qobject_cast<KFileItemModel *>(view->model()));
    KFileItemModel *fileItemModel = static_cast<KFileItemModel *>(view->model());

    const KFileItem item = fileItemModel->fileItem(index);
    return item.isLink();
}

QString KFileItemListWidgetInformant::roleText(const QByteArray &role, const QHash<QByteArray, QVariant> &values) const
{
    QString text;
    const QVariant roleValue = values.value(role);
    QLocale local;
    KFormat formatter(local);

    // Implementation note: In case if more roles require a custom handling
    // use a hash + switch for a linear runtime.

    auto formatDate = [formatter, local](const QDateTime &time) {
        if (ContentDisplaySettings::useShortRelativeDates()) {
            return formatter.formatRelativeDateTime(time, QLocale::ShortFormat);
        } else {
            return local.toString(time, QLocale::ShortFormat);
        }
    };

    if (role == "size") {
        if (values.value("isDir").toBool()) {
            if (!roleValue.isNull() && roleValue != -1) {
                // The item represents a directory.
                if (ContentDisplaySettings::directorySizeMode() == ContentDisplaySettings::EnumDirectorySizeMode::ContentCount
                    || roleValue == -2 /* size is invalid */) {
                    //  Show the number of sub directories instead of the file size of the directory.
                    const int count = values.value("count").toInt();
                    text = i18ncp("@item:intable", "%1 item", "%1 items", count);
                } else {
                    // if we have directory size available
                    const KIO::filesize_t size = roleValue.value<KIO::filesize_t>();
                    text = formatter.formatByteSize(size);
                }
            }
        } else {
            const KIO::filesize_t size = roleValue.value<KIO::filesize_t>();
            text = formatter.formatByteSize(size);
        }
    } else if (role == "modificationtime" || role == "creationtime" || role == "accesstime") {
        bool ok;
        const long long time = roleValue.toLongLong(&ok);
        if (ok && time != -1) {
            const QDateTime dateTime = QDateTime::fromSecsSinceEpoch(time);
            text = formatDate(dateTime);
        }
    } else if (role == "deletiontime" || role == "imageDateTime") {
        const QDateTime dateTime = roleValue.toDateTime();
        if (dateTime.isValid()) {
            text = formatDate(dateTime);
        }
    } else if (role == "dimensions") {
        const auto dimensions = roleValue.toSize();
        if (dimensions.isValid()) {
            text = i18nc("width × height", "%1 × %2", dimensions.width(), dimensions.height());
        }
    } else if (role == "permissions") {
        const auto permissions = roleValue.value<QVariantList>();

        switch (ContentDisplaySettings::usePermissionsFormat()) {
        case ContentDisplaySettings::EnumUsePermissionsFormat::SymbolicFormat:
            text = permissions.at(0).toString();
            break;
        case ContentDisplaySettings::EnumUsePermissionsFormat::NumericFormat:
            text = QString::number(permissions.at(1).toInt(), 8);
            break;
        case ContentDisplaySettings::EnumUsePermissionsFormat::CombinedFormat:
            text = QLatin1String("%1 (%2)").arg(permissions.at(0).toString()).arg(permissions.at(1).toInt(), 0, 8);
            break;
        }
    } else {
        text = KStandardItemListWidgetInformant::roleText(role, values);
    }

    return text;
}

QFont KFileItemListWidgetInformant::customizedFontForLinks(const QFont &baseFont) const
{
    // The customized font should be italic if the file is a symbolic link.
    QFont font(baseFont);
    font.setItalic(true);
    return font;
}

KFileItemListWidget::KFileItemListWidget(KItemListWidgetInformant *informant, QGraphicsItem *parent)
    : KStandardItemListWidget(informant, parent)
{
}

KFileItemListWidget::~KFileItemListWidget()
{
}

KItemListWidgetInformant *KFileItemListWidget::createInformant()
{
    return new KFileItemListWidgetInformant();
}

bool KFileItemListWidget::isRoleRightAligned(const QByteArray &role) const
{
    return role == "size" || role == "permissions";
}

bool KFileItemListWidget::isHidden() const
{
    return data().value("isHidden").toBool();
}

QFont KFileItemListWidget::customizedFont(const QFont &baseFont) const
{
    // The customized font should be italic if the file is a symbolic link.
    QFont font(baseFont);
    font.setItalic(data().value("isLink").toBool());
    return font;
}

int KFileItemListWidget::selectionLength(const QString &text) const
{
    // If item is a directory, use the whole text length for
    // selection (ignore all points)
    if (data().value("isDir").toBool()) {
        return numberOfUnicodeCharactersIn(text);
    }

    int indexOfExtension = text.length();

    QMimeDatabase db;
    const QString extension = db.suffixForFileName(text);
    if (extension.isEmpty()) {
        // For an unknown extension just exclude the extension after
        // the last point. This does not work for multiple extensions like
        // *.tar.gz but usually this is anyhow a known extension.
        indexOfExtension = text.lastIndexOf(QLatin1Char('.'));

        // If no point could be found, use whole text length for selection.
        if (indexOfExtension < 1) {
            indexOfExtension = text.length();
        }

    } else {
        indexOfExtension -= extension.length() + 1;
    }

    return numberOfUnicodeCharactersIn(text.left(indexOfExtension));
}

void KFileItemListWidget::hoverSequenceStarted()
{
    KFileItemListView *view = listView();

    if (!view) {
        return;
    }

    const QUrl itemUrl = data().value("url").toUrl();

    view->setHoverSequenceState(itemUrl, 0);
}

void KFileItemListWidget::forceUpdate()
{
    updateAdditionalInfoTextColor();
    // icon layout does not include the icons in the item selection rectangle
    // so its icon does not need updating
    if (listView()->itemLayout() != KStandardItemListView::ItemLayout::IconsLayout) {
        invalidateIconCache();
    }
    update();
}

void KFileItemListWidget::hoverSequenceIndexChanged(int sequenceIndex)
{
    KFileItemListView *view = listView();

    if (!view) {
        return;
    }

    const QUrl itemUrl = data().value("url").toUrl();

    view->setHoverSequenceState(itemUrl, sequenceIndex);

    // Force-update the displayed icon
    invalidateIconCache();
    update();
}

void KFileItemListWidget::hoverSequenceEnded()
{
    KFileItemListView *view = listView();

    if (!view) {
        return;
    }

    view->setHoverSequenceState(QUrl(), 0);
}

KFileItemListView *KFileItemListWidget::listView()
{
    return dynamic_cast<KFileItemListView *>(parentItem());
}

#include "moc_kfileitemlistwidget.cpp"
