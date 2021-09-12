/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2020 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef SINGLECLICKSELECTIONPROXYSTYLE_H
#define SINGLECLICKSELECTIONPROXYSTYLE_H

#include <QProxyStyle>

/**
 * @todo write docs
 */
class SingleClickSelectionProxyStyle : public QProxyStyle
{
public:
    inline int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                  const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override
    {
        if (hint == QStyle::SH_ItemView_ActivateItemOnSingleClick) {
            return 0;
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

#endif // SINGLECLICKSELECTIONPROXYSTYLE_H
