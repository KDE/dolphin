/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINSETTINGSDIALOG_H
#define DOLPHINSETTINGSDIALOG_H

#include <KPageDialog>
#include <KActionCollection>

class QUrl;
class SettingsPageBase;

/**
 * @brief Settings dialog for Dolphin.
 *
 * Contains the pages for Startup, View Modes, Navigation, Services, General, and Trash.
 */
class DolphinSettingsDialog : public KPageDialog
{
    Q_OBJECT

public:
    explicit DolphinSettingsDialog(const QUrl& url, QWidget* parent = nullptr, KActionCollection* actions = {});
    ~DolphinSettingsDialog() override;

    KPageWidgetItem* trashSettings;

Q_SIGNALS:
    void settingsChanged();

private Q_SLOTS:
    /** Enables the Apply button. */
    void enableApply();
    void applySettings();
    void restoreDefaults();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    static SettingsPageBase *createTrashSettingsPage(QWidget *parent);

    QList<SettingsPageBase*> m_pages;
    bool m_unsavedChanges;
};

#endif
