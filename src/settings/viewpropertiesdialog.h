/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 * SPDX-FileCopyrightText: 2018 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VIEWPROPERTIESDIALOG_H
#define VIEWPROPERTIESDIALOG_H

#include "dolphin_export.h"

#include <QDialog>

class QCheckBox;
class QListWidget;
class QListWidgetItem;
class QComboBox;
class QPushButton;
class QRadioButton;
class ViewProperties;
class DolphinView;

/**
 * @brief Dialog for changing the current view properties of a directory.
 *
 * It is possible to specify the view mode, the sorting order, whether hidden files
 * and previews should be shown. The properties can be assigned to the current folder,
 * or recursively to all sub folders.
 */
class DOLPHIN_EXPORT ViewPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewPropertiesDialog(DolphinView *dolphinView);
    /**
     * Edits the display style that every folder without a style of its own is shown with.
     */
    explicit ViewPropertiesDialog(QWidget *parent);
    ~ViewPropertiesDialog() override;

public Q_SLOTS:
    void accept() override;

private Q_SLOTS:
    void slotApply();
    void slotViewModeChanged(int index);
    void slotSortingChanged(int index);
    void slotSortOrderChanged(int index);
    void slotGroupedSortingChanged();
    void slotSortFoldersFirstChanged();
    void slotSortHiddenLastChanged();
    void slotShowPreviewChanged();
    void slotShowHiddenFilesChanged();
    void slotItemChanged(QListWidgetItem *item);
    void markAsDirty(bool isDirty);

Q_SIGNALS:
    void isDirtyChanged(bool isDirty);
    /** Is emitted when the default display style has been written. */
    void defaultPropertiesChanged();

private:
    /** @p dolphinView is null where the dialog edits the default display style. */
    ViewPropertiesDialog(QWidget *parent, DolphinView *dolphinView);

    void applyViewProperties();
    void loadSettings();

private:
    bool m_isDirty;
    DolphinView *m_dolphinView;
    ViewProperties *m_viewProps;

    QComboBox *m_viewMode;
    QComboBox *m_sortOrder;
    QComboBox *m_sorting;
    QCheckBox *m_sortFoldersFirst;
    QCheckBox *m_sortHiddenLast;
    QCheckBox *m_previewsShown;
    QCheckBox *m_showInGroups;
    QCheckBox *m_showHiddenFiles;
    QRadioButton *m_applyToCurrentFolder;
    QRadioButton *m_applyToSubFolders;
    QRadioButton *m_applyToAllFolders;
    QCheckBox *m_useAsDefault;
    QListWidget *m_listWidget;
};

#endif
