/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef SINGLECLICKSELECTIONPROXYSTYLE_H
#define SINGLECLICKSELECTIONPROXYSTYLE_H

#include <QProxyStyle>

namespace SelectionMode
{

/**
 * @brief A simple proxy style to temporarily make single click select and not activate
 *
 * @see QProxyStyle
 */
class SingleClickSelectionProxyStyle : public QProxyStyle
{
public:
    inline int
    styleHint(StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override
    {
        if (hint == QStyle::SH_ItemView_ActivateItemOnSingleClick) {
            return 0;
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

}

#endif // SINGLECLICKSELECTIONPROXYSTYLE_H
