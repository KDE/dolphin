/*
 * SPDX-FileCopyrightText: 2009-2010 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef VCSSETTINGSTAB_H
#define VCSSETTINGSTAB_H

#include <QStringList>
#include <QWidget>

#include <KPluginMetaData>

class QListView;
class QString;
class ServiceModel;

/**
 * @brief Configuration for version control plugins.
 */
class VcsSettingsTab : public QWidget
{
    Q_OBJECT

public:
    explicit VcsSettingsTab(const QVector<KPluginMetaData>& plugins, QWidget* parent = nullptr);
    ~VcsSettingsTab() override;

    void applySettings();
    void restoreDefaultSettings();

Q_SIGNALS:
    void changed();

private:
    /**
     * Adds a row to the model of m_listView.
     */
    void addRow(const QString &icon,
                const QString &text,
                const QString &value,
                bool checked);

private:
    ServiceModel *m_serviceModel;
    QListView* m_listView;
    QStringList m_enabledVcsPlugins;
};

#endif // VCSSETTINGSTAB_H
