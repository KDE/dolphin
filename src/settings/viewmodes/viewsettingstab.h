/*
 * SPDX-FileCopyrightText: 2008-2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEWSETTINGSTAB_H
#define VIEWSETTINGSTAB_H

#include "settings/settingspagebase.h"

class DolphinFontRequester;
class QComboBox;
class QCheckBox;
class QSlider;
class QSpinBox;
class QRadioButton;

/**
 * @brief Represents one tab of the view-settings page.
 */
class ViewSettingsTab : public SettingsPageBase
{
    Q_OBJECT

public:
    enum Mode { IconsMode, CompactMode, DetailsMode };

    explicit ViewSettingsTab(Mode mode, QWidget *parent = nullptr);
    ~ViewSettingsTab() override;

    void applySettings() override;
    void restoreDefaults() override;

private Q_SLOTS:

    void slotDefaultSliderMoved(int value);
    void slotPreviewSliderMoved(int value);

private:
    void loadSettings();
    void showToolTip(QSlider *slider, int value);

private:
    Mode m_mode;
    QSlider *m_defaultSizeSlider;
    QSlider *m_previewSizeSlider;

    DolphinFontRequester *m_fontRequester;
    QComboBox *m_widthBox;
    QComboBox *m_maxLinesBox;
    QRadioButton *m_entireRow;
    QRadioButton *m_iconAndNameOnly;
    QCheckBox *m_expandableFolders;
};

#endif
