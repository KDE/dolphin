/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef BACKGROUNDCOLORHELPER_H
#define BACKGROUNDCOLORHELPER_H

#include <QColor>
#include <QPointer>

#include <memory>

class QWidget;

namespace SelectionMode
{

/**
 * @brief A Singleton class for managing the colors of selection mode widgets.
 */
class BackgroundColorHelper
{
public:
    static BackgroundColorHelper *instance();

    /**
     * Changes the background color of @p widget to a distinct color scheme matching color which makes it clear that the widget belongs to the selection mode.
     * The background color of @p widget will from now on be updated automatically when the palette of the application changes.
     */
    void controlBackgroundColor(QWidget *widget);

private:
    BackgroundColorHelper();

    /**
     * Called when the palette of the application changes.
     * Triggers updateBackgroundColor() and the updates the background color of m_colorControlledWidgets.
     * @see updateBackgroundColor
     */
    void slotPaletteChanged();

    /** Calculates a new m_colorControlledWidgets based on the current colour scheme of the application. */
    void updateBackgroundColor();

private:
    /// The widgets who have given up control over the background color to BackgroundColorHelper.
    std::vector<QPointer<QWidget>> m_colorControlledWidgets;
    /// The color to be used for the widgets' backgrounds.
    QColor m_backgroundColor;

    /// Singleton object
    static BackgroundColorHelper *s_instance;
};

}

#endif // BACKGROUNDCOLORHELPER_H
