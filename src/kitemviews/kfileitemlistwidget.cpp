/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfileitemlistwidget.h"
#include "kfileitemmodel.h"
#include "kitemlistview.h"

#include "dolphin_detailsmodesettings.h"

#include <KFormat>
#include <KLocalizedString>

#include <QMimeDatabase>

KFileItemListWidgetInformant::KFileItemListWidgetInformant() :
    KStandardItemListWidgetInformant()
{
}

KFileItemListWidgetInformant::~KFileItemListWidgetInformant()
{
}

QString KFileItemListWidgetInformant::itemText(int index, const KItemListView* view) const
{
    Q_ASSERT(qobject_cast<KFileItemModel*>(view->model()));
    KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(view->model());

    const KFileItem item = fileItemModel->fileItem(index);
    return item.text();
}

bool KFileItemListWidgetInformant::itemIsLink(int index, const KItemListView* view) const
{
    Q_ASSERT(qobject_cast<KFileItemModel*>(view->model()));
    KFileItemModel* fileItemModel = static_cast<KFileItemModel*>(view->model());

    const KFileItem item = fileItemModel->fileItem(index);
    return item.isLink();
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
            // The item represents a directory.
            if (!roleValue.isNull()) {
                const int count = values.value("count").toInt();
                if (count > 0) {
                    if (DetailsModeSettings::directorySizeCount()) {
                        //  Show the number of sub directories instead of the file size of the directory.
                        text = i18ncp("@item:intable", "%1 item", "%1 items", count);
                    } else {
                        // if we have directory size available
                        if (roleValue != -1) {
                            const KIO::filesize_t size = roleValue.value<KIO::filesize_t>();
                            text = KFormat().formatByteSize(size);
                        }
                    }
                }
            }
        } else {
            const KIO::filesize_t size = roleValue.value<KIO::filesize_t>();
            text = KFormat().formatByteSize(size);
        }
    } else if (role == "modificationtime" || role == "creationtime" || role == "accesstime") {
            bool ok;
            const long long time = roleValue.toLongLong(&ok);
            if (ok && time != -1) {
                return QLocale().toString(QDateTime::fromSecsSinceEpoch(time), QLocale::ShortFormat);
            }
    } else if (role == "deletiontime" || role == "imageDateTime") {
        const QDateTime dateTime = roleValue.toDateTime();
        text = QLocale().toString(dateTime, QLocale::ShortFormat);
    } else {
        text = KStandardItemListWidgetInformant::roleText(role, values);
    }

    return text;
}

QFont KFileItemListWidgetInformant::customizedFontForLinks(const QFont& baseFont) const
{
    // The customized font should be italic if the file is a symbolic link.
    QFont font(baseFont);
    font.setItalic(true);
    return font;
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

bool KFileItemListWidget::isHidden() const
{
    return data().value("isHidden").toBool();
}

QFont KFileItemListWidget::customizedFont(const QFont& baseFont) const
{
    // The customized font should be italic if the file is a symbolic link.
    QFont font(baseFont);
    font.setItalic(data().value("isLink").toBool());
    return font;
}

int KFileItemListWidget::selectionLength(const QString& text) const
{
    // Select the text without MIME-type extension
    int selectionLength = text.length();

    // If item is a directory, use the whole text length for
    // selection (ignore all points)
    if(data().value("isDir").toBool()) {
        return selectionLength;
    }

    QMimeDatabase db;
    const QString extension = db.suffixForFileName(text);
    if (extension.isEmpty()) {
        // For an unknown extension just exclude the extension after
        // the last point. This does not work for multiple extensions like
        // *.tar.gz but usually this is anyhow a known extension.
        selectionLength = text.lastIndexOf(QLatin1Char('.'));

        // If no point could be found, use whole text length for selection.
        if (selectionLength < 1) {
            selectionLength = text.length();
        }

    } else {
        selectionLength -= extension.length() + 1;
    }

    return selectionLength;
}

