/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef CONFIGUREPREVIEWPLUGINDIALOG_H
#define CONFIGUREPREVIEWPLUGINDIALOG_H

#include <QDialog>

/**
 * @brief Dialog for configuring preview-plugins.
 */
class ConfigurePreviewPluginDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param pluginName       User visible name of the plugin
     * @param desktopEntryName The name of the plugin that is noted in the desktopentry.
     *                         Is used to instantiate the plugin to get the configuration
     *                         widget.
     * @param parent           Parent widget.
     */
    ConfigurePreviewPluginDialog(const QString& pluginName,
                                 const QString& desktopEntryName,
                                 QWidget* parent);
    ~ConfigurePreviewPluginDialog() override = default;
};

#endif
