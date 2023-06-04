/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2020 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DOLPHINURLNAVIGATOR_H
#define DOLPHINURLNAVIGATOR_H

#include <KUrlNavigator>

/**
 * @brief Extends KUrlNavigator in a Dolphin-specific way.
 *
 * Makes sure that Dolphin preferences and settings are
 * applied to all constructed DolphinUrlNavigators.
 * @see KUrlNavigator
 *
 * To apply changes to all instances of this class @see DolphinUrlNavigatorsController.
 */
class DolphinUrlNavigator : public KUrlNavigator
{
    Q_OBJECT

public:
    /**
     * Applies all Dolphin-specific settings to a KUrlNavigator
     * @see KUrlNavigator::KurlNavigator()
     */
    DolphinUrlNavigator(QWidget *parent = nullptr);

    /**
     * Applies all Dolphin-specific settings to a KUrlNavigator
     * @see KUrlNavigator::KurlNavigator()
     */
    DolphinUrlNavigator(const QUrl &url, QWidget *parent = nullptr);

    ~DolphinUrlNavigator() override;

    // TODO: Fix KUrlNavigator::sizeHint() instead.
    QSize sizeHint() const override;

    /**
     * Wraps the visual state of a DolphinUrlNavigator so it can be passed around.
     * This notably doesn't involve the locationUrl or history.
     */
    struct VisualState {
        bool isUrlEditable;
        bool hasFocus;
        QString text;
        int cursorPosition;
        int selectionStart;
        int selectionLength;
    };
    /**
     * Retrieve the visual state of this DolphinUrlNavigator.
     * If two DolphinUrlNavigators have the same visual state they should look identical.
     *
     * @return a copy of the visualState of this object. Ownership of this copy is transferred
     *         to the caller via std::unique_ptr.
     */
    std::unique_ptr<VisualState> visualState() const;
    /**
     * @param visualState A struct describing the new visual state of this object.
     */
    void setVisualState(const VisualState &visualState);

    /**
     * Clears the text in the text field
     */
    void clearText() const;

    /**
     * Displays placeholder text in the URL navigator
     */
    void setPlaceholderText(const QString &text);

public Q_SLOTS:
    /**
     * Switches to "breadcrumb" mode if the editable mode is not set to be
     * preferred in the Dolphin settings.
     */
    void slotReturnPressed();
};

#endif // DOLPHINURLNAVIGATOR_H
