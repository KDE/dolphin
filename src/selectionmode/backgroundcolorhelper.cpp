/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "backgroundcolorhelper.h"

#include <KColorScheme>

#include <QGuiApplication>
#include <QPalette>
#include <QWidget>

using namespace SelectionMode;

BackgroundColorHelper *BackgroundColorHelper::instance()
{
    if (!s_instance) {
        s_instance = new BackgroundColorHelper;
    }
    return s_instance;
}

void setBackgroundColorForWidget(QWidget *widget, QColor color)
{
    QPalette palette;
    palette.setBrush(QPalette::Active, QPalette::Window, color);
    palette.setBrush(QPalette::Inactive, QPalette::Window, color);
    palette.setBrush(QPalette::Disabled, QPalette::Window, color);
    widget->setAutoFillBackground(true);
    widget->setPalette(palette);
}

void BackgroundColorHelper::controlBackgroundColor(QWidget *widget)
{
    setBackgroundColorForWidget(widget, m_backgroundColor);

    Q_ASSERT_X(std::find(m_colorControlledWidgets.begin(), m_colorControlledWidgets.end(), widget) == m_colorControlledWidgets.end(),
               "controlBackgroundColor",
               "Duplicate insertion is not necessary because the background color should already automatically update itself on paletteChanged");
    m_colorControlledWidgets.emplace_back(widget);
}

bool BackgroundColorHelper::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (event->type() == QEvent::ApplicationPaletteChange) {
        slotPaletteChanged();
    }
    return false;
}

BackgroundColorHelper::BackgroundColorHelper()
{
    updateBackgroundColor();
    qApp->installEventFilter(this);
}

void BackgroundColorHelper::slotPaletteChanged()
{
    updateBackgroundColor();
    for (auto i = m_colorControlledWidgets.begin(); i != m_colorControlledWidgets.end(); ++i) {
        while (!*i) {
            i = m_colorControlledWidgets.erase(i);
            if (i == m_colorControlledWidgets.end()) {
                return;
            }
        }
        setBackgroundColorForWidget(*i, m_backgroundColor);
    }
}

void BackgroundColorHelper::updateBackgroundColor()
{
    // We use colors from the color scheme for mixing so it fits the theme.
    const auto colorScheme = KColorScheme(QPalette::Normal, KColorScheme::Window);
    const auto activeBackgroundColor = colorScheme.background(KColorScheme::BackgroundRole::ActiveBackground).color();
    // We use the positive color for mixing so the end product doesn't look like a warning or error.
    const auto positiveBackgroundColor = colorScheme.background(KColorScheme::BackgroundRole::PositiveBackground).color();

    // Make sure the new background color has a meaningfully different hue than the activeBackgroundColor.
    const int hueDifference = positiveBackgroundColor.hue() - activeBackgroundColor.hue();
    int newHue;
    if (std::abs(hueDifference) > 80) {
        newHue = (activeBackgroundColor.hue() + positiveBackgroundColor.hue()) / 2;
    } else {
        newHue = hueDifference > 0 ? activeBackgroundColor.hue() + 40 : activeBackgroundColor.hue() - 40;
        newHue %= 360; // hue needs to be between 0 and 359 per Qt documentation.
    }

    m_backgroundColor = QColor::fromHsv(newHue,
                                        // Saturation should be closer to the saturation of the active color
                                        // because otherwise the selection mode color might overpower it.
                                        .7 * activeBackgroundColor.saturation() + .3 * positiveBackgroundColor.saturation(),
                                        (activeBackgroundColor.value() + positiveBackgroundColor.value()) / 2,
                                        (activeBackgroundColor.alpha() + positiveBackgroundColor.alpha()) / 2);
}

BackgroundColorHelper *BackgroundColorHelper::s_instance = nullptr;
