/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PREVIEWSSETTINGSPAGE_H
#define PREVIEWSSETTINGSPAGE_H

#include <kiowidgets_export.h>

#include "settings/settingspagebase.h"

class QCheckBox;
class QSpinBox;
class QTreeView;
class QModelIndex;

/**
 * @brief Allows the configuration of file previews.
 */
class PreviewsSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit PreviewsSettingsPage(QWidget *parent);
    ~PreviewsSettingsPage() override;

    /**
     * Applies the general settings for the view modes
     * The settings are persisted automatically when
     * closing Dolphin.
     */
    void applySettings() override;

    /** Restores the settings to default values. */
    void restoreDefaults() override;

protected:
    void showEvent(QShowEvent *event) override;

private Q_SLOTS:

private:
    void loadPreviewPlugins();
    void loadSettings();

private:
    bool m_initialized;
    QTreeView *m_treeView;
    QStringList m_enabledPreviewPlugins;
    QSpinBox *m_localFileSizeBox;
    QSpinBox *m_remoteFileSizeBox;
    QCheckBox *m_enableRemoteFolderThumbnail;
};

#endif
