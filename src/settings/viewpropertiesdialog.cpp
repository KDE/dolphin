/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "viewpropertiesdialog.h"

#include "additionalinfodialog.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "kitemviews/kfileitemmodel.h"
#include "viewpropsprogressinfo.h"
#include "views/dolphinview.h"

#include <KComboBox>
#include <KLocalizedString>
#include <KMessageBox>
#include <KWindowConfig>

#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>

#include <views/viewproperties.h>

ViewPropertiesDialog::ViewPropertiesDialog(DolphinView* dolphinView) :
    QDialog(dolphinView),
    m_isDirty(false),
    m_dolphinView(dolphinView),
    m_viewProps(nullptr),
    m_viewMode(nullptr),
    m_sortOrder(nullptr),
    m_sorting(nullptr),
    m_sortFoldersFirst(nullptr),
    m_previewsShown(nullptr),
    m_showInGroups(nullptr),
    m_showHiddenFiles(nullptr),
    m_additionalInfo(nullptr),
    m_applyToCurrentFolder(nullptr),
    m_applyToSubFolders(nullptr),
    m_applyToAllFolders(nullptr),
    m_useAsDefault(nullptr)
{
    Q_ASSERT(dolphinView);
    const bool useGlobalViewProps = GeneralSettings::globalViewProps();

    setWindowTitle(i18nc("@title:window", "View Properties"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    const QUrl& url = dolphinView->url();
    m_viewProps = new ViewProperties(url);
    m_viewProps->setAutoSaveEnabled(false);

    auto layout = new QVBoxLayout(this);
    setLayout(layout);

    auto propsGrid = new QWidget(this);
    layout->addWidget(propsGrid);

    // create 'Properties' group containing view mode, sorting, sort order and show hidden files
    QWidget* propsBox = this;
    if (!useGlobalViewProps) {
        propsBox = new QGroupBox(i18nc("@title:group", "Properties"), this);
        layout->addWidget(propsBox);
    }

    QLabel* viewModeLabel = new QLabel(i18nc("@label:listbox", "View mode:"), propsGrid);
    m_viewMode = new KComboBox(propsGrid);
    m_viewMode->addItem(QIcon::fromTheme(QStringLiteral("view-list-icons")), i18nc("@item:inlistbox", "Icons"), DolphinView::IconsView);
    m_viewMode->addItem(QIcon::fromTheme(QStringLiteral("view-list-details")), i18nc("@item:inlistbox", "Compact"), DolphinView::CompactView);
    m_viewMode->addItem(QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@item:inlistbox", "Details"), DolphinView::DetailsView);

    QLabel* sortingLabel = new QLabel(i18nc("@label:listbox", "Sorting:"), propsGrid);
    QWidget* sortingBox = new QWidget(propsGrid);

    m_sortOrder = new KComboBox(sortingBox);
    m_sortOrder->addItem(i18nc("@item:inlistbox Sort", "Ascending"));
    m_sortOrder->addItem(i18nc("@item:inlistbox Sort", "Descending"));

    m_sorting = new KComboBox(sortingBox);
    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
        m_sorting->addItem(info.translation, info.role);
    }

    m_sortFoldersFirst = new QCheckBox(i18nc("@option:check", "Show folders first"));
    m_previewsShown = new QCheckBox(i18nc("@option:check", "Show preview"));
    m_showInGroups = new QCheckBox(i18nc("@option:check", "Show in groups"));
    m_showHiddenFiles = new QCheckBox(i18nc("@option:check", "Show hidden files"));

    m_additionalInfo = new QPushButton(i18nc("@action:button", "Additional Information"));

    QHBoxLayout* sortingLayout = new QHBoxLayout();
    sortingLayout->setMargin(0);
    sortingLayout->addWidget(m_sortOrder);
    sortingLayout->addWidget(m_sorting);
    sortingBox->setLayout(sortingLayout);

    QGridLayout* propsGridLayout = new QGridLayout(propsGrid);
    propsGridLayout->addWidget(viewModeLabel, 0, 0, Qt::AlignRight);
    propsGridLayout->addWidget(m_viewMode, 0, 1);
    propsGridLayout->addWidget(sortingLabel, 1, 0, Qt::AlignRight);
    propsGridLayout->addWidget(sortingBox, 1, 1);

    QVBoxLayout* propsBoxLayout = propsBox == this ? layout : new QVBoxLayout(propsBox);
    propsBoxLayout->addWidget(propsGrid);
    propsBoxLayout->addWidget(m_sortFoldersFirst);
    propsBoxLayout->addWidget(m_previewsShown);
    propsBoxLayout->addWidget(m_showInGroups);
    propsBoxLayout->addWidget(m_showHiddenFiles);
    propsBoxLayout->addWidget(m_additionalInfo);

    connect(m_viewMode, static_cast<void(KComboBox::*)(int)>(&KComboBox::currentIndexChanged),
            this, &ViewPropertiesDialog::slotViewModeChanged);
    connect(m_sorting, static_cast<void(KComboBox::*)(int)>(&KComboBox::currentIndexChanged),
            this, &ViewPropertiesDialog::slotSortingChanged);
    connect(m_sortOrder, static_cast<void(KComboBox::*)(int)>(&KComboBox::currentIndexChanged),
            this, &ViewPropertiesDialog::slotSortOrderChanged);
    connect(m_additionalInfo, &QPushButton::clicked,
            this, &ViewPropertiesDialog::configureAdditionalInfo);
    connect(m_sortFoldersFirst, &QCheckBox::clicked,
            this, &ViewPropertiesDialog::slotSortFoldersFirstChanged);
    connect(m_previewsShown, &QCheckBox::clicked,
            this, &ViewPropertiesDialog::slotShowPreviewChanged);
    connect(m_showInGroups, &QCheckBox::clicked,
            this, &ViewPropertiesDialog::slotGroupedSortingChanged);
    connect(m_showHiddenFiles, &QCheckBox::clicked,
            this, &ViewPropertiesDialog::slotShowHiddenFilesChanged);

    // Only show the following settings if the view properties are remembered
    // for each directory:
    if (!useGlobalViewProps) {
        // create 'Apply View Properties To' group
        QGroupBox* applyBox = new QGroupBox(i18nc("@title:group", "Apply View Properties To"), this);
        layout->addWidget(applyBox);

        m_applyToCurrentFolder = new QRadioButton(i18nc("@option:radio Apply View Properties To",
                                                        "Current folder"), applyBox);
        m_applyToCurrentFolder->setChecked(true);
        m_applyToSubFolders = new QRadioButton(i18nc("@option:radio Apply View Properties To",
                                                     "Current folder including all sub-folders"), applyBox);
        m_applyToAllFolders = new QRadioButton(i18nc("@option:radio Apply View Properties To",
                                                     "All folders"), applyBox);

        QButtonGroup* applyGroup = new QButtonGroup(this);
        applyGroup->addButton(m_applyToCurrentFolder);
        applyGroup->addButton(m_applyToSubFolders);
        applyGroup->addButton(m_applyToAllFolders);

        QVBoxLayout* applyBoxLayout = new QVBoxLayout(applyBox);
        applyBoxLayout->addWidget(m_applyToCurrentFolder);
        applyBoxLayout->addWidget(m_applyToSubFolders);
        applyBoxLayout->addWidget(m_applyToAllFolders);

        m_useAsDefault = new QCheckBox(i18nc("@option:check", "Use these view properties as default"), this);
        layout->addWidget(m_useAsDefault);

        connect(m_applyToCurrentFolder, &QRadioButton::clicked,
                this, &ViewPropertiesDialog::markAsDirty);
        connect(m_applyToSubFolders, &QRadioButton::clicked,
                this, &ViewPropertiesDialog::markAsDirty);
        connect(m_applyToAllFolders, &QRadioButton::clicked,
                this, &ViewPropertiesDialog::markAsDirty);
        connect(m_useAsDefault, &QCheckBox::clicked,
                this, &ViewPropertiesDialog::markAsDirty);
    }

    layout->addStretch();

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ViewPropertiesDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ViewPropertiesDialog::reject);
    layout->addWidget(buttonBox);

    auto okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setShortcut(Qt::CTRL + Qt::Key_Return);
    okButton->setDefault(true);

    auto applyButton = buttonBox->button(QDialogButtonBox::Apply);
    connect(applyButton, &QPushButton::clicked, this, &ViewPropertiesDialog::slotApply);
    connect(this, &ViewPropertiesDialog::isDirtyChanged, applyButton, [applyButton](bool isDirty) {
        applyButton->setEnabled(isDirty);
    });

    const KConfigGroup dialogConfig(KSharedConfig::openConfig(QStringLiteral("dolphinrc")), "ViewPropertiesDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), dialogConfig);

    loadSettings();
}

ViewPropertiesDialog::~ViewPropertiesDialog()
{
    m_isDirty = false;
    delete m_viewProps;
    m_viewProps = nullptr;

    KConfigGroup dialogConfig(KSharedConfig::openConfig(QStringLiteral("dolphinrc")), "ViewPropertiesDialog");
    KWindowConfig::saveWindowSize(windowHandle(), dialogConfig);
}

void ViewPropertiesDialog::accept()
{
    applyViewProperties();
    QDialog::accept();
}

void ViewPropertiesDialog::slotApply()
{
    applyViewProperties();
    markAsDirty(false);
}

void ViewPropertiesDialog::slotViewModeChanged(int index)
{
    const QVariant itemData = m_viewMode->itemData(index);
    const DolphinView::Mode viewMode = static_cast<DolphinView::Mode>(itemData.toInt());
    m_viewProps->setViewMode(viewMode);
    markAsDirty(true);
}

void ViewPropertiesDialog::slotSortingChanged(int index)
{
    const QByteArray role = m_sorting->itemData(index).toByteArray();
    m_viewProps->setSortRole(role);
    markAsDirty(true);
}

void ViewPropertiesDialog::slotSortOrderChanged(int index)
{
    const Qt::SortOrder sortOrder = (index == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
    m_viewProps->setSortOrder(sortOrder);
    markAsDirty(true);
}

void ViewPropertiesDialog::slotGroupedSortingChanged()
{
    m_viewProps->setGroupedSorting(m_showInGroups->isChecked());
    markAsDirty(true);
}

void ViewPropertiesDialog::slotSortFoldersFirstChanged()
{
    const bool foldersFirst = m_sortFoldersFirst->isChecked();
    m_viewProps->setSortFoldersFirst(foldersFirst);
    markAsDirty(true);
}

void ViewPropertiesDialog::slotShowPreviewChanged()
{
    const bool show = m_previewsShown->isChecked();
    m_viewProps->setPreviewsShown(show);
    markAsDirty(true);
}

void ViewPropertiesDialog::slotShowHiddenFilesChanged()
{
    const bool show = m_showHiddenFiles->isChecked();
    m_viewProps->setHiddenFilesShown(show);
    markAsDirty(true);
}

void ViewPropertiesDialog::markAsDirty(bool isDirty)
{
    if (m_isDirty != isDirty) {
        m_isDirty = isDirty;
        emit isDirtyChanged(isDirty);
    }
}

void ViewPropertiesDialog::configureAdditionalInfo()
{
    QList<QByteArray> visibleRoles = m_viewProps->visibleRoles();
    const bool useDefaultRoles = (m_viewProps->viewMode() == DolphinView::DetailsView) && visibleRoles.isEmpty();
    if (useDefaultRoles) {
        // Using the details view without any additional information (-> additional column)
        // makes no sense and leads to a usability problem as no viewport area is available
        // anymore. Hence as fallback provide at least a size and date column.
        visibleRoles.clear();
        visibleRoles.append("text");
        visibleRoles.append("size");
        visibleRoles.append("modificationtime");
        m_viewProps->setVisibleRoles(visibleRoles);
    }

    QPointer<AdditionalInfoDialog> dialog = new AdditionalInfoDialog(this, visibleRoles);
    if (dialog->exec() == QDialog::Accepted) {
        m_viewProps->setVisibleRoles(dialog->visibleRoles());
        markAsDirty(true);
    }
    delete dialog;
}

void ViewPropertiesDialog::applyViewProperties()
{
    // if nothing changed in the dialog, we have nothing to apply
    if (!m_isDirty) {
        return;
    }

    const bool applyToSubFolders = m_applyToSubFolders && m_applyToSubFolders->isChecked();
    if (applyToSubFolders) {
        const QString text(i18nc("@info", "The view properties of all sub-folders will be changed. Do you want to continue?"));
        if (KMessageBox::questionYesNo(this, text) == KMessageBox::No) {
            return;
        }

        ViewPropsProgressInfo* info = new ViewPropsProgressInfo(m_dolphinView,
                                                                m_dolphinView->url(),
                                                                *m_viewProps);
        info->setAttribute(Qt::WA_DeleteOnClose);
        info->setWindowModality(Qt::NonModal);
        info->show();
    }

    const bool applyToAllFolders = m_applyToAllFolders && m_applyToAllFolders->isChecked();

    // If the user selected 'Apply To All Folders' the view properties implicitely
    // are also used as default for new folders.
    const bool useAsDefault = applyToAllFolders || (m_useAsDefault && m_useAsDefault->isChecked());
    if (useAsDefault) {
        // For directories where no .directory file is available, the .directory
        // file stored for the global view properties is used as fallback. To update
        // this file we temporary turn on the global view properties mode.
        Q_ASSERT(!GeneralSettings::globalViewProps());

        GeneralSettings::setGlobalViewProps(true);
        ViewProperties defaultProps(m_dolphinView->url());
        defaultProps.setDirProperties(*m_viewProps);
        defaultProps.save();
        GeneralSettings::setGlobalViewProps(false);
    }

    if (applyToAllFolders) {
        const QString text(i18nc("@info", "The view properties of all folders will be changed. Do you want to continue?"));
        if (KMessageBox::questionYesNo(this, text) == KMessageBox::No) {
            return;
        }

        // Updating the global view properties time stamp in the general settings makes
        // all existing viewproperties invalid, as they have a smaller time stamp.
        GeneralSettings* settings = GeneralSettings::self();
        settings->setViewPropsTimestamp(QDateTime::currentDateTime());
        settings->save();
    }

    m_dolphinView->setMode(m_viewProps->viewMode());
    m_dolphinView->setSortRole(m_viewProps->sortRole());
    m_dolphinView->setSortOrder(m_viewProps->sortOrder());
    m_dolphinView->setSortFoldersFirst(m_viewProps->sortFoldersFirst());
    m_dolphinView->setGroupedSorting(m_viewProps->groupedSorting());
    m_dolphinView->setVisibleRoles(m_viewProps->visibleRoles());
    m_dolphinView->setPreviewsShown(m_viewProps->previewsShown());
    m_dolphinView->setHiddenFilesShown(m_viewProps->hiddenFilesShown());

    m_viewProps->save();

    markAsDirty(false);
}

void ViewPropertiesDialog::loadSettings()
{
    // Load view mode
    switch (m_viewProps->viewMode()) {
    case DolphinView::IconsView:   m_viewMode->setCurrentIndex(0); break;
    case DolphinView::CompactView: m_viewMode->setCurrentIndex(1); break;
    case DolphinView::DetailsView: m_viewMode->setCurrentIndex(2); break;
    default: break;
    }

    // Load sort order and sorting
    const int sortOrderIndex = (m_viewProps->sortOrder() == Qt::AscendingOrder) ? 0 : 1;
    m_sortOrder->setCurrentIndex(sortOrderIndex);

    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    int sortRoleIndex = 0;
    for (int i = 0; i < rolesInfo.count(); ++i) {
        if (rolesInfo[i].role == m_viewProps->sortRole()) {
            sortRoleIndex = i;
            break;
        }
    }
    m_sorting->setCurrentIndex(sortRoleIndex);

    m_sortFoldersFirst->setChecked(m_viewProps->sortFoldersFirst());

    // Load show preview, show in groups and show hidden files settings
    m_previewsShown->setChecked(m_viewProps->previewsShown());
    m_showInGroups->setChecked(m_viewProps->groupedSorting());
    m_showHiddenFiles->setChecked(m_viewProps->hiddenFilesShown());
    markAsDirty(false);
}

