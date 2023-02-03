/*
 * SPDX-FileCopyrightText: 2009-2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CONTEXTMENUSETTINGSPAGE_H
#define CONTEXTMENUSETTINGSPAGE_H

#include "settings/settingspagebase.h"

#include <KActionCollection>

#include <QString>

class QListView;
class QSortFilterProxyModel;
class ServiceModel;
class QLineEdit;

/**
 * @brief Configurations for services in the context menu.
 */
class ContextMenuSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit ContextMenuSettingsPage(QWidget *parent, const KActionCollection *actions, const QStringList &actionIds);
    ~ContextMenuSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

protected:
    void showEvent(QShowEvent *event) override;

private Q_SLOTS:
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
    void addRow(const QString &icon, const QString &text, const QString &value, bool checked);
    bool entryVisible(const QString &id);
    void setEntryVisible(const QString &id, bool visible);

private:
    bool m_initialized;
    ServiceModel *m_serviceModel;
    QSortFilterProxyModel *m_sortModel;
    QListView *m_listView;
    QLineEdit *m_searchLineEdit;
    QStringList m_enabledVcsPlugins;
    const KActionCollection *m_actions;
    const QStringList m_actionIds;
};

#endif
