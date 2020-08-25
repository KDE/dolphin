/*
 * SPDX-FileCopyrightText: 2009-2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef SERVICESSETTINGSPAGE_H
#define SERVICESSETTINGSPAGE_H

#include "settings/settingspagebase.h"

#include <QString>

class QListView;
class QSortFilterProxyModel;
class ServiceModel;
class QLineEdit;

/**
 * @brief Page for the 'Services' settings of the Dolphin settings dialog.
 */
class ServicesSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit ServicesSettingsPage(QWidget* parent);
    ~ServicesSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    /**
     * Loads locally installed services.
     */
    void loadServices();

private:
    /**
     * Loads installed version control systems.
     */
    void loadVersionControlSystems();

    bool isInServicesList(const QString &service) const;

    /**
     * Adds a row to the model of m_listView.
     */
    void addRow(const QString &icon,
                const QString &text,
                const QString &value,
                bool checked);

private:
    bool m_initialized;
    ServiceModel *m_serviceModel;
    QSortFilterProxyModel *m_sortModel;
    QListView* m_listView;
    QLineEdit *m_searchLineEdit;
    QStringList m_enabledVcsPlugins;
};

#endif
