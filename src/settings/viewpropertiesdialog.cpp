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
#include "views/dolphinview.h"
#include "settings/dolphinsettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "viewpropsprogressinfo.h"

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
#include <nepomuk/resourcemanager.h>
#endif

#include <KComponentData>
#include <KLocale>
#include <KIconLoader>
#include <KIO/NetAccess>
#include <KMessageBox>
#include <KStandardDirs>
#include <KUrl>
#include <kcombobox.h>

#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QBoxLayout>

#include <views/dolphinsortfilterproxymodel.h>
#include <views/viewproperties.h>

ViewPropertiesDialog::ViewPropertiesDialog(DolphinView* dolphinView) :
    KDialog(dolphinView),
    m_isDirty(false),
    m_dolphinView(dolphinView),
    m_viewProps(0),
    m_viewMode(0),
    m_sortOrder(0),
    m_sorting(0),
    m_sortFoldersFirst(0),
    m_showPreview(0),
    m_showInGroups(0),
    m_showHiddenFiles(0),
    m_additionalInfo(0),
    m_applyToCurrentFolder(0),
    m_applyToSubFolders(0),
    m_applyToAllFolders(0),
    m_useAsDefault(0)
{
    Q_ASSERT(dolphinView != 0);
    const bool useGlobalViewProps = DolphinSettings::instance().generalSettings()->globalViewProps();

    setCaption(i18nc("@title:window", "View Properties"));
    setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

    const KUrl& url = dolphinView->url();
    m_viewProps = new ViewProperties(url);
    m_viewProps->setAutoSaveEnabled(false);

    QWidget* main = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout();

    // create 'Properties' group containing view mode, sorting, sort order and show hidden files
    QWidget* propsBox = main;
    if (!useGlobalViewProps) {
        propsBox = new QGroupBox(i18nc("@title:group", "Properties"), main);
    }

    QWidget* propsGrid = new QWidget();

    QLabel* viewModeLabel = new QLabel(i18nc("@label:listbox", "View mode:"), propsGrid);
    m_viewMode = new KComboBox(propsGrid);
    m_viewMode->addItem(KIcon("view-list-icons"), i18nc("@item:inlistbox", "Icons"));
    m_viewMode->addItem(KIcon("view-list-details"), i18nc("@item:inlistbox", "Details"));
    m_viewMode->addItem(KIcon("view-file-columns"), i18nc("@item:inlistbox", "Column"));

    QLabel* sortingLabel = new QLabel(i18nc("@label:listbox", "Sorting:"), propsGrid);
    QWidget* sortingBox = new QWidget(propsGrid);

    m_sortOrder = new KComboBox(sortingBox);
    m_sortOrder->addItem(i18nc("@item:inlistbox Sort", "Ascending"));
    m_sortOrder->addItem(i18nc("@item:inlistbox Sort", "Descending"));

    m_sorting = new KComboBox(sortingBox);
    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Name"));
    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Size"));
    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Date"));
    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Permissions"));
    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Owner"));
    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Group"));
    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Type"));
#ifdef HAVE_NEPOMUK
    // TODO: Hided "sort by rating" and "sort by tags" as without caching the performance
    // is too slow currently (Nepomuk will support caching in future releases).
    //
    // if (!Nepomuk::ResourceManager::instance()->init()) {
    //    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Rating"));
    //    m_sorting->addItem(i18nc("@item:inlistbox Sort", "By Tags"));
    // }
#endif
    m_sortFoldersFirst = new QCheckBox(i18nc("@option:check", "Show folders first"));
    m_showPreview = new QCheckBox(i18nc("@option:check", "Show preview"));
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

    QVBoxLayout* propsBoxLayout = new QVBoxLayout(propsBox);
    propsBoxLayout->addWidget(propsGrid);
    propsBoxLayout->addWidget(m_sortFoldersFirst);
    propsBoxLayout->addWidget(m_showPreview);
    propsBoxLayout->addWidget(m_showInGroups);
    propsBoxLayout->addWidget(m_showHiddenFiles);
    propsBoxLayout->addWidget(m_additionalInfo);

    topLayout->addWidget(propsBox);

    connect(m_viewMode, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotViewModeChanged(int)));
    connect(m_sorting, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSortingChanged(int)));
    connect(m_sortOrder, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSortOrderChanged(int)));
    connect(m_additionalInfo, SIGNAL(clicked()),
            this, SLOT(configureAdditionalInfo()));
    connect(m_sortFoldersFirst, SIGNAL(clicked()),
            this, SLOT(slotSortFoldersFirstChanged()));
    connect(m_showPreview, SIGNAL(clicked()),
            this, SLOT(slotShowPreviewChanged()));
    connect(m_showInGroups, SIGNAL(clicked()),
            this, SLOT(slotCategorizedSortingChanged()));
    connect(m_showHiddenFiles, SIGNAL(clicked()),
            this, SLOT(slotShowHiddenFilesChanged()));

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));

    // Only show the following settings if the view properties are remembered
    // for each directory:
    if (!useGlobalViewProps) {
        // create 'Apply View Properties To' group
        QGroupBox* applyBox = new QGroupBox(i18nc("@title:group", "Apply View Properties To"), main);

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

        m_useAsDefault = new QCheckBox(i18nc("@option:check", "Use these view properties as default"), main);

        topLayout->addWidget(applyBox);
        topLayout->addWidget(m_useAsDefault);

        connect(m_applyToCurrentFolder, SIGNAL(clicked(bool)),
                this, SLOT(markAsDirty(bool)));
        connect(m_applyToSubFolders, SIGNAL(clicked(bool)),
                this, SLOT(markAsDirty(bool)));
        connect(m_applyToAllFolders, SIGNAL(clicked(bool)),
                this, SLOT(markAsDirty(bool)));
        connect(m_useAsDefault, SIGNAL(clicked(bool)),
                this, SLOT(markAsDirty(bool)));
    }

    main->setLayout(topLayout);
    setMainWidget(main);

    const KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                                    "ViewPropertiesDialog");
    restoreDialogSize(dialogConfig);

    loadSettings();
}

ViewPropertiesDialog::~ViewPropertiesDialog()
{
    m_isDirty = false;
    delete m_viewProps;
    m_viewProps = 0;

    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "ViewPropertiesDialog");
    saveDialogSize(dialogConfig, KConfigBase::Persistent);
}

void ViewPropertiesDialog::slotOk()
{
    applyViewProperties();
    accept();
}

void ViewPropertiesDialog::slotApply()
{
    applyViewProperties();
    markAsDirty(false);
}

void ViewPropertiesDialog::slotViewModeChanged(int index)
{
    m_viewProps->setViewMode(static_cast<DolphinView::Mode>(index));
    markAsDirty(true);

    const DolphinView::Mode mode = m_viewProps->viewMode();
    m_showInGroups->setEnabled(mode == DolphinView::IconsView);
    m_additionalInfo->setEnabled(mode != DolphinView::ColumnView);
}

void ViewPropertiesDialog::slotSortingChanged(int index)
{
    const DolphinView::Sorting sorting = DolphinSortFilterProxyModel::sortingForColumn(index);
    m_viewProps->setSorting(sorting);
    markAsDirty(true);
}

void ViewPropertiesDialog::slotSortOrderChanged(int index)
{
    const Qt::SortOrder sortOrder = (index == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
    m_viewProps->setSortOrder(sortOrder);
    markAsDirty(true);
}

void ViewPropertiesDialog::slotCategorizedSortingChanged()
{
    m_viewProps->setCategorizedSorting(m_showInGroups->isChecked());
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
    const bool show = m_showPreview->isChecked();
    m_viewProps->setShowPreview(show);
    markAsDirty(true);
}

void ViewPropertiesDialog::slotShowHiddenFilesChanged()
{
    const bool show = m_showHiddenFiles->isChecked();
    m_viewProps->setShowHiddenFiles(show);
    markAsDirty(true);
}

void ViewPropertiesDialog::markAsDirty(bool isDirty)
{
    m_isDirty = isDirty;
    enableButtonApply(isDirty);
}

void ViewPropertiesDialog::configureAdditionalInfo()
{
    KFileItemDelegate::InformationList info = m_viewProps->additionalInfo();
    const bool useDefaultInfo = (m_viewProps->viewMode() == DolphinView::DetailsView) &&
                                (info.isEmpty() || info.contains(KFileItemDelegate::NoInformation));
    if (useDefaultInfo) {
        // Using the details view without any additional information (-> additional column)
        // makes no sense and leads to a usability problem as no viewport area is available
        // anymore. Hence as fallback provide at least a size and date column.
        info.clear();
        info.append(KFileItemDelegate::Size);
        info.append(KFileItemDelegate::ModificationTime);
        m_viewProps->setAdditionalInfo(info);
    }

    QPointer<AdditionalInfoDialog> dialog = new AdditionalInfoDialog(this, info);
    if (dialog->exec() == QDialog::Accepted) {
        m_viewProps->setAdditionalInfo(dialog->informationList());
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

    const bool applyToSubFolders = (m_applyToSubFolders != 0) &&
                                   m_applyToSubFolders->isChecked();
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

    const bool applyToAllFolders = (m_applyToAllFolders != 0) &&
                                   m_applyToAllFolders->isChecked();

    // If the user selected 'Apply To All Folders' the view properties implicitely
    // are also used as default for new folders.
    const bool useAsDefault = applyToAllFolders ||
                              ((m_useAsDefault != 0) && m_useAsDefault->isChecked());
    if (useAsDefault) {
        // For directories where no .directory file is available, the .directory
        // file stored for the global view properties is used as fallback. To update
        // this file we temporary turn on the global view properties mode.
        GeneralSettings* settings = DolphinSettings::instance().generalSettings();
        Q_ASSERT(!settings->globalViewProps());

        settings->setGlobalViewProps(true);
        ViewProperties defaultProps(m_dolphinView->url());
        defaultProps.setDirProperties(*m_viewProps);
        defaultProps.save();
        settings->setGlobalViewProps(false);
    }

    if (applyToAllFolders) {
        const QString text(i18nc("@info", "The view properties of all folders will be changed. Do you want to continue?"));
        if (KMessageBox::questionYesNo(this, text) == KMessageBox::No) {
            return;
        }

        // Updating the global view properties time stamp in the general settings makes
        // all existing viewproperties invalid, as they have a smaller time stamp.
        GeneralSettings* settings = DolphinSettings::instance().generalSettings();
        settings->setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    m_dolphinView->setMode(m_viewProps->viewMode());
    m_dolphinView->setSorting(m_viewProps->sorting());
    m_dolphinView->setSortOrder(m_viewProps->sortOrder());
    m_dolphinView->setSortFoldersFirst(m_viewProps->sortFoldersFirst());
    m_dolphinView->setCategorizedSorting(m_viewProps->categorizedSorting());
    m_dolphinView->setAdditionalInfo(m_viewProps->additionalInfo());
    m_dolphinView->setShowPreview(m_viewProps->showPreview());
    m_dolphinView->setShowHiddenFiles(m_viewProps->showHiddenFiles());

    m_viewProps->save();

    markAsDirty(false);
}

void ViewPropertiesDialog::loadSettings()
{
    // load view mode
    const int index = static_cast<int>(m_viewProps->viewMode());
    m_viewMode->setCurrentIndex(index);

    // load sort order and sorting
    const int sortOrderIndex = (m_viewProps->sortOrder() == Qt::AscendingOrder) ? 0 : 1;
    m_sortOrder->setCurrentIndex(sortOrderIndex);
    m_sorting->setCurrentIndex(m_viewProps->sorting());

    const bool enabled = (index == DolphinView::DetailsView) ||
                         (index == DolphinView::IconsView);
    m_additionalInfo->setEnabled(enabled);

    m_sortFoldersFirst->setChecked(m_viewProps->sortFoldersFirst());
    // load show preview, show in groups and show hidden files settings
    m_showPreview->setChecked(m_viewProps->showPreview());

    m_showInGroups->setChecked(m_viewProps->categorizedSorting());
    m_showInGroups->setEnabled(index == DolphinView::IconsView); // only the icons view supports categorized sorting

    m_showHiddenFiles->setChecked(m_viewProps->showHiddenFiles());
    markAsDirty(false);
}

#include "viewpropertiesdialog.moc"
