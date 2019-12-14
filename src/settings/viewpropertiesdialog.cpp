/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2018 by Elvis Angelaccio <elvis.angelaccio@kde.org>     *
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

#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "global.h"
#include "kitemviews/kfileitemmodel.h"
#include "viewpropsprogressinfo.h"
#include "views/dolphinview.h"

#include <KCollapsibleGroupBox>
#include <KLocalizedString>
#include <KMessageBox>
#include <KWindowConfig>

#ifdef HAVE_BALOO
    #include <Baloo/IndexerConfig>
#endif

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>

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
    m_applyToCurrentFolder(nullptr),
    m_applyToSubFolders(nullptr),
    m_applyToAllFolders(nullptr),
    m_useAsDefault(nullptr)
{
    Q_ASSERT(dolphinView);
    const bool useGlobalViewProps = GeneralSettings::globalViewProps();

    setWindowTitle(i18nc("@title:window", "View Display Style"));

    const QUrl& url = dolphinView->url();
    m_viewProps = new ViewProperties(url);
    m_viewProps->setAutoSaveEnabled(false);

    auto layout = new QFormLayout(this);
    // Otherwise the dialog won't resize when we collapse the KCollapsibleGroupBox.
    layout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(layout);

    // create 'Properties' group containing view mode, sorting, sort order and show hidden files
    m_viewMode = new QComboBox();
    m_viewMode->addItem(QIcon::fromTheme(QStringLiteral("view-list-icons")), i18nc("@item:inlistbox", "Icons"), DolphinView::IconsView);
    m_viewMode->addItem(QIcon::fromTheme(QStringLiteral("view-list-details")), i18nc("@item:inlistbox", "Compact"), DolphinView::CompactView);
    m_viewMode->addItem(QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@item:inlistbox", "Details"), DolphinView::DetailsView);

    m_sortOrder = new QComboBox();
    m_sortOrder->addItem(i18nc("@item:inlistbox Sort", "Ascending"));
    m_sortOrder->addItem(i18nc("@item:inlistbox Sort", "Descending"));

    m_sorting = new QComboBox();
    const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
    foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
        m_sorting->addItem(info.translation, info.role);
    }

    m_sortFoldersFirst = new QCheckBox(i18nc("@option:check", "Show folders first"));
    m_previewsShown = new QCheckBox(i18nc("@option:check", "Show preview"));
    m_showInGroups = new QCheckBox(i18nc("@option:check", "Show in groups"));
    m_showHiddenFiles = new QCheckBox(i18nc("@option:check", "Show hidden files"));

    auto additionalInfoBox = new KCollapsibleGroupBox();
    additionalInfoBox->setTitle(i18nc("@title:group", "Additional Information"));
    auto innerLayout = new QVBoxLayout();

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

        // Add checkboxes
        bool indexingEnabled = false;
#ifdef HAVE_BALOO
        Baloo::IndexerConfig config;
        indexingEnabled = config.fileIndexingEnabled();
#endif

        m_listWidget = new QListWidget();
        connect(m_listWidget, &QListWidget::itemChanged, this, &ViewPropertiesDialog::slotItemChanged);
        m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
        const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
        foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
            QListWidgetItem* item = new QListWidgetItem(info.translation, m_listWidget);
            item->setCheckState(visibleRoles.contains(info.role) ? Qt::Checked : Qt::Unchecked);

            const bool enable = ((!info.requiresBaloo && !info.requiresIndexer) ||
                                (info.requiresBaloo) ||
                                (info.requiresIndexer && indexingEnabled)) && info.role != "text";

            if (!enable) {
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            }
        }
        QLabel* additionalViewOptionsLabel = new QLabel(i18n("Choose what to see on each file or folder:"));
        innerLayout->addWidget(additionalViewOptionsLabel);
        innerLayout->addWidget(m_listWidget);
    }

    additionalInfoBox->setLayout(innerLayout);

    QHBoxLayout* sortingLayout = new QHBoxLayout();
    sortingLayout->setContentsMargins(0, 0, 0, 0);
    sortingLayout->addWidget(m_sortOrder);
    sortingLayout->addWidget(m_sorting);

    layout->addRow(i18nc("@label:listbox", "View mode:"), m_viewMode);
    layout->addRow(i18nc("@label:listbox", "Sorting:"), sortingLayout);

    layout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    layout->addRow(i18n("View options:"), m_sortFoldersFirst);
    layout->addRow(QString(), m_previewsShown);
    layout->addRow(QString(), m_showInGroups);
    layout->addRow(QString(), m_showHiddenFiles);

    connect(m_viewMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ViewPropertiesDialog::slotViewModeChanged);
    connect(m_sorting, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ViewPropertiesDialog::slotSortingChanged);
    connect(m_sortOrder, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ViewPropertiesDialog::slotSortOrderChanged);
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
        m_applyToCurrentFolder = new QRadioButton(i18nc("@option:radio Apply View Properties To",
                                                        "Current folder"));
        m_applyToCurrentFolder->setChecked(true);
        m_applyToSubFolders = new QRadioButton(i18nc("@option:radio Apply View Properties To",
                                                     "Current folder and sub-folders"));
        m_applyToAllFolders = new QRadioButton(i18nc("@option:radio Apply View Properties To",
                                                     "All folders"));

        QButtonGroup* applyGroup = new QButtonGroup(this);
        applyGroup->addButton(m_applyToCurrentFolder);
        applyGroup->addButton(m_applyToSubFolders);
        applyGroup->addButton(m_applyToAllFolders);

        layout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

        layout->addRow(i18nc("@title:group", "Apply to:"), m_applyToCurrentFolder);
        layout->addRow(QString(), m_applyToSubFolders);
        layout->addRow(QString(), m_applyToAllFolders);
        layout->addRow(QString(), m_applyToAllFolders);

        m_useAsDefault = new QCheckBox(i18nc("@option:check", "Use as default view settings"), this);
        layout->addRow(QString(), m_useAsDefault);

        connect(m_applyToCurrentFolder, &QRadioButton::clicked,
                this, &ViewPropertiesDialog::markAsDirty);
        connect(m_applyToSubFolders, &QRadioButton::clicked,
                this, &ViewPropertiesDialog::markAsDirty);
        connect(m_applyToAllFolders, &QRadioButton::clicked,
                this, &ViewPropertiesDialog::markAsDirty);
        connect(m_useAsDefault, &QCheckBox::clicked,
                this, &ViewPropertiesDialog::markAsDirty);
    }

    layout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    layout->addRow(additionalInfoBox);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ViewPropertiesDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ViewPropertiesDialog::reject);
    layout->addWidget(buttonBox);

    auto okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setShortcut(Qt::CTRL + Qt::Key_Return);
    okButton->setDefault(true);

    auto applyButton = buttonBox->button(QDialogButtonBox::Apply);
    applyButton->setEnabled(false);
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

void ViewPropertiesDialog::slotItemChanged(QListWidgetItem *item)
{
    Q_UNUSED(item)
    markAsDirty(true);
}

void ViewPropertiesDialog::markAsDirty(bool isDirty)
{
    if (m_isDirty != isDirty) {
        m_isDirty = isDirty;
        emit isDirtyChanged(isDirty);
    }
}

void ViewPropertiesDialog::applyViewProperties()
{
    // if nothing changed in the dialog, we have nothing to apply
    if (!m_isDirty) {
        return;
    }

    // Update visible roles.
    {
        QList<QByteArray> visibleRoles;
        int index = 0;
        const QList<KFileItemModel::RoleInfo> rolesInfo = KFileItemModel::rolesInformation();
        foreach (const KFileItemModel::RoleInfo& info, rolesInfo) {
            const QListWidgetItem* item = m_listWidget->item(index);
             if (item->checkState() == Qt::Checked) {
                visibleRoles.append(info.role);
            }
            ++index;
        }

        m_viewProps->setVisibleRoles(visibleRoles);
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

    // If the user selected 'Apply To All Folders' the view properties implicitly
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

