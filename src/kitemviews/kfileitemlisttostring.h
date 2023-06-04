/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KFILEITEMLISTTOSTRING_H
#define KFILEITEMLISTTOSTRING_H

class KFileItemList;
class QFontMetrics;
class QString;

enum ItemsState { None, Selected };

/**
 * @brief Generates a textual representation of the given list of KFileItems.
 *
 * This method is especially useful to be very explicit about which items will be affected by an action.
 * For example can a label for a delete button be enriched to say "Permanantly Delete `picture1` and `picture2`"
 * but also "Permanently Delete 7 Selected Folders" without requiring a huge amount of new translations for this.
 *
 * Unfortunately this doesn't always work.
 *
 * For some texts and some languages this wide range of words cannot be inserted into a text while staying
 * grammatically correct. Because of this you will probably need to write a fallback for these languages.
 * Something like this:
 * \code
 * // i18n: @action:inmenu menu with actions like copy, paste, rename.
 * // %2 is a textual representation of the currently selected files or folders. This can be the name of
 * // the file/files like "file1" or "file1, file2 and file3" or an aggregate like "8 Selected Folders".
 * // If this sort of word puzzle can not be correctly translated in your language, translate it as "NULL" (without the quotes)
 * // and a fallback will be used.
 * auto buttonText = i18ncp("@action A more elaborate and clearly worded version of the Delete action", "Permanently Delete %2", "Permanently Delete %2", items.count(), fileItemListToString(items, fontMetrics.averageCharWidth() * 20, fontMetrics));
 * if (buttonText == QStringLiteral("NULL")) {
 *      buttonText = i18ncp("@action Delete button label. %1 is the number of items to be deleted.",
 *              "Permanently Delete %1 Item", "Permanently Delete %1 Items", items.count())
 * }
 * \endcode
 * (The i18n call should be completely in the line following the i18n: comment without any line breaks within the i18n call or the comment might not be correctly extracted. See: https://commits.kde.org/kxmlgui/a31135046e1b3335b5d7bbbe6aa9a883ce3284c1 )
 *
 * @param items The KFileItemList for which a QString should be generated.
 * @param maximumTextWidth The maximum width/horizontalAdvance the QString should have. Keep scaling in mind.
 * @param fontMetrics the fontMetrics for the font that is used to calculate the maximumTextWidth.
 * @param itemsState A state of the @p items that should be spelled out in the returned QString.
 * @returns the names of the @p items separated by commas if that is below the @p maximumCharacterWidth.
 *          Otherwise returns something like "5 Files", "8 Selected Folders" or "60 Items"
 *          while being as specific as possible.
 */
QString fileItemListToString(KFileItemList items, int maximumTextWidth, const QFontMetrics &fontMetrics, ItemsState itemsState = ItemsState::None);

#endif // KFILEITEMLISTTOSTRING_H
