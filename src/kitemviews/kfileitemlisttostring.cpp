/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kfileitemlisttostring.h"

#include <KFileItem>
#include <KFileItemListProperties>
#include <KLocalizedString>

#include <QFontMetrics>
#include <QString>

QString fileItemListToString(KFileItemList items, int maximumTextWidth, const QFontMetrics &fontMetrics, ItemsState itemsState)
{
    QString text;
    switch (items.count()) {
    case 1:
        text = i18nc("Textual representation of a file. %1 is the name of the file/folder.",
                "\"%1\"", items.first().name());
        break;
    case 2:
        text = i18nc("Textual representation of two files. %1 and %2 are names of files/folders.",
                "\"%1\" and \"%2\"", items.first().name(), items.last().name());
        break;
    case 3:
        text = i18nc("Textual representation of three files. %1, %2 and %3 are names of files/folders.",
                "\"%1\", \"%2\" and \"%3\"",
                items.first().name(), items.at(1).name(), items.last().name());
        break;
    case 4:
        text = i18nc("Textual representation of four files. %1, %2, %3 and %4 are names of files/folders.",
                "\"%1\", \"%2\", \"%3\" and \"%4\"",
                items.first().name(), items.at(1).name(), items.at(2).name(), items.last().name());
        break;
    case 5:
        text = i18nc("Textual representation of five files. %1, %2, %3, %4 and %5 are names of files/folders.",
                "\"%1\", \"%2\", \"%3\", \"%4\" and \"%5\"",
                items.first().name(), items.at(1).name(), items.at(2).name(), items.at(3).name(), items.last().name());
        break;
    default:
        text = QString();
        break;
    }

    // At some point the added clarity from the text starts being less important than the text width.
    if (!text.isEmpty() && fontMetrics.horizontalAdvance(text) <= maximumTextWidth) {
        return text;
    }

    const KFileItemListProperties properties(items);
    if (itemsState == Selected) {
        if (properties.isFile()) {
            text = i18ncp("Textual representation of selected files. %1 is the number of files.",
                    "One Selected File", "%1 Selected Files", items.count());
        } else if (properties.isDirectory()) {
            text = i18ncp("Textual representation of selected folders. %1 is the number of folders.",
                    "One Selected Folder", "%1 Selected Folders", items.count());
        } else {
            text = i18ncp("Textual representation of selected fileitems. %1 is the number of files/folders.",
                    "One Selected Item", "%1 Selected Items", items.count());
        }

        if (fontMetrics.horizontalAdvance(text) <= maximumTextWidth) {
            return text;
        }
    }

    if (properties.isFile()) {
        return i18ncp("Textual representation of files. %1 is the number of files.",
                "One File", "%1 Files", items.count());
    } else if (properties.isDirectory()) {
        return i18ncp("Textual representation of folders. %1 is the number of folders.",
                "One Folder", "%1 Folders", items.count());
    } else {
        return i18ncp("Textual representation of fileitems. %1 is the number of files/folders.",
                "One Item", "%1 Items", items.count());
    }
}
