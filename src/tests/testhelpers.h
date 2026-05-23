/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <QApplication>
#include <QTest>

namespace TestHelpers
{

/**
 * Disables all Qt UI animations.
 * Call once in initTestCase().
 * Eliminates the need for qWait(N) hacks that wait for animations to finish.
 */
inline void disableAnimations()
{
    QApplication::setEffectEnabled(Qt::UI_AnimateMenu, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateToolBox, false);
}

} // namespace TestHelpers
