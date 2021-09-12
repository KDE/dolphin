/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef BACKGROUNDCOLORHELPER_H
#define BACKGROUNDCOLORHELPER_H

#include <QColor>
#include <QPointer>

#include <memory>

class QWidget;

/**
 * @brief A Singleton class for managing the colors of selection mode widgets.
 */
class BackgroundColorHelper
{
public:
    static BackgroundColorHelper *instance();

    /**
     * Changes the background color of @p widget to a distinct color scheme matching color which makes it clear that the widget belongs to the selection mode.
     */
    void controlBackgroundColor(QWidget *widget);

private:
    BackgroundColorHelper();

    void slotPaletteChanged();

    void updateBackgroundColor();

private:
    std::vector<QPointer<QWidget>> m_colorControlledWidgets;
    QColor m_backgroundColor;

    static BackgroundColorHelper *s_instance;
};

#endif // BACKGROUNDCOLORHELPER_H
