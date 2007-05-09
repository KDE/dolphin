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
#include "viewpropsprogressinfo.h"
#include "dolphinview.h"
#include "dolphinsettings.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphin_generalsettings.h"
#include "viewproperties.h"

#include <kcomponentdata.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>
#include <QtGui/QBoxLayout>

ViewPropertiesDialog::ViewPropertiesDialog(DolphinView* dolphinView) :
    KDialog(dolphinView),
    m_isDirty(false),
    m_dolphinView(dolphinView),
    m_viewProps(0),
    m_viewMode(0),
    m_sorting(0),
    m_sortOrder(0),
    m_categorizedSorting(0),
    m_additionalInfo(0),
    m_showPreview(0),
    m_showHiddenFiles(0),
    m_applyToCurrentFolder(0),
    m_applyToSubFolders(0),
    m_useAsDefault(0)
{
    Q_ASSERT(dolphinView != 0);
    const bool useGlobalViewProps = DolphinSettings::instance().generalSettings()->globalViewProps();

    setCaption(i18n("View Properties"));
    setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

    const KUrl& url = dolphinView->url();
    m_viewProps = new ViewProperties(url);
    m_viewProps->setAutoSaveEnabled(false);

    QWidget* main = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout();

    // create 'Properties' group containing view mode, sorting, sort order and show hidden files
    QWidget* propsBox = main;
    if (!useGlobalViewProps) {
        propsBox = new QGroupBox(i18n("Properties"), main);
    }

    QLabel* viewModeLabel = new QLabel(i18n("View mode:"), propsBox);
    m_viewMode = new QComboBox(propsBox);
    m_viewMode->addItem(KIcon("view-icon"), i18n("Icons"));
    m_viewMode->addItem(KIcon("fileview-text"), i18n("Details"));
    m_viewMode->addItem(KIcon("fileview-column"), i18n("Column"));
    const int index = static_cast<int>(m_viewProps->viewMode());
    m_viewMode->setCurrentIndex(index);
    const bool iconsViewEnabled = (index == DolphinView::IconsView);

    QLabel* sortingLabel = new QLabel(i18n("Sorting:"), propsBox);
    QWidget* sortingBox = new QWidget(propsBox);

    m_sortOrder = new QComboBox(sortingBox);
    m_sortOrder->addItem(i18n("Ascending"));
    m_sortOrder->addItem(i18n("Descending"));
    const int sortOrderIdx = (m_viewProps->sortOrder() == Qt::AscendingOrder) ? 0 : 1;
    m_sortOrder->setCurrentIndex(sortOrderIdx);

    m_categorizedSorting = new QComboBox(sortingBox);
    m_categorizedSorting->addItem(i18n("Uncategorized"));
    m_categorizedSorting->addItem(i18n("Categorized"));
    m_categorizedSorting->setCurrentIndex(m_viewProps->categorizedSorting() ? 1 : 0);
    m_categorizedSorting->setEnabled(iconsViewEnabled);

    m_sorting = new QComboBox(sortingBox);
    m_sorting->addItem("By Name");
    m_sorting->addItem("By Size");
    m_sorting->addItem("By Date");
    m_sorting->addItem("By Permissions");
    m_sorting->addItem("By Owner");
    m_sorting->addItem("By Group");
    m_sorting->addItem("By Type");
    m_sorting->setCurrentIndex(m_viewProps->sorting());

    QHBoxLayout* sortingLayout = new QHBoxLayout();
    sortingLayout->setMargin(0);
    sortingLayout->addWidget(m_sortOrder);
    sortingLayout->addWidget(m_sorting);
    sortingLayout->addWidget(m_categorizedSorting);
    sortingBox->setLayout(sortingLayout);

    QLabel* additionalInfoLabel = new QLabel(i18n("Additional information:"), propsBox);
    m_additionalInfo = new QComboBox(propsBox);
    m_additionalInfo->addItem(i18n("No Information"), KFileItemDelegate::NoInformation);
    m_additionalInfo->addItem(i18n("Type"), KFileItemDelegate::FriendlyMimeType);
    m_additionalInfo->addItem(i18n("Size"), KFileItemDelegate::Size);
    m_additionalInfo->addItem(i18n("Date"), KFileItemDelegate::ModificationTime);
    const int addInfoIndex = m_additionalInfo->findData(m_viewProps->additionalInfo());
    m_additionalInfo->setCurrentIndex(addInfoIndex);
    m_additionalInfo->setEnabled(iconsViewEnabled);

    m_showPreview = new QCheckBox(i18n("Show preview"), propsBox);
    m_showPreview->setChecked(m_viewProps->showPreview());

    m_showHiddenFiles = new QCheckBox(i18n("Show hidden files"), propsBox);
    m_showHiddenFiles->setChecked(m_viewProps->showHiddenFiles());

    QGridLayout* propsBoxLayout = new QGridLayout(propsBox);
    propsBoxLayout->addWidget(viewModeLabel, 0, 0);
    propsBoxLayout->addWidget(m_viewMode, 0, 1);
    propsBoxLayout->addWidget(sortingLabel, 1, 0);
    propsBoxLayout->addWidget(sortingBox, 1, 1);
    propsBoxLayout->addWidget(additionalInfoLabel, 2, 0);
    propsBoxLayout->addWidget(m_additionalInfo, 2, 1);
    propsBoxLayout->addWidget(m_showPreview, 3, 0);
    propsBoxLayout->addWidget(m_showHiddenFiles, 4, 0);

    topLayout->addWidget(propsBox);

    connect(m_viewMode, SIGNAL(activated(int)),
            this, SLOT(slotViewModeChanged(int)));
    connect(m_sorting, SIGNAL(activated(int)),
            this, SLOT(slotSortingChanged(int)));
    connect(m_sortOrder, SIGNAL(activated(int)),
            this, SLOT(slotSortOrderChanged(int)));
    connect(m_categorizedSorting, SIGNAL(activated(int)),
            this, SLOT(slotCategorizedSortingChanged(int)));
    connect(m_additionalInfo, SIGNAL(activated(int)),
            this, SLOT(slotAdditionalInfoChanged(int)));
    connect(m_showPreview, SIGNAL(clicked()),
            this, SLOT(slotShowPreviewChanged()));
    connect(m_showHiddenFiles, SIGNAL(clicked()),
            this, SLOT(slotShowHiddenFilesChanged()));

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));

    // Only show the following settings if the view properties are remembered
    // for each directory:
    if (!useGlobalViewProps) {
        // create 'Apply View Properties To' group
        QGroupBox* applyBox = new QGroupBox(i18n("Apply View Properties To"), main);

        m_applyToCurrentFolder = new QRadioButton(i18n("Current folder"), applyBox);
        m_applyToCurrentFolder->setChecked(true);
        m_applyToSubFolders = new QRadioButton(i18n("Current folder including all sub folders"), applyBox);
        m_applyToAllFolders = new QRadioButton(i18n("All folders"), applyBox);

        QButtonGroup* applyGroup = new QButtonGroup(this);
        applyGroup->addButton(m_applyToCurrentFolder);
        applyGroup->addButton(m_applyToSubFolders);
        applyGroup->addButton(m_applyToAllFolders);

        QVBoxLayout* applyBoxLayout = new QVBoxLayout(applyBox);
        applyBoxLayout->addWidget(m_applyToCurrentFolder);
        applyBoxLayout->addWidget(m_applyToSubFolders);
        applyBoxLayout->addWidget(m_applyToAllFolders);

        m_useAsDefault = new QCheckBox(i18n("Use as default for new folders"), main);

        topLayout->addWidget(applyBox);
        topLayout->addWidget(m_useAsDefault);

        connect(m_applyToCurrentFolder, SIGNAL(clicked()),
                this, SLOT(markAsDirty()));
        connect(m_applyToSubFolders, SIGNAL(clicked()),
                this, SLOT(markAsDirty()));
        connect(m_applyToAllFolders, SIGNAL(clicked()),
                this, SLOT(markAsDirty()));
        connect(m_useAsDefault, SIGNAL(clicked()),
                this, SLOT(markAsDirty()));
    }

    main->setLayout(topLayout);
    setMainWidget(main);
}

ViewPropertiesDialog::~ViewPropertiesDialog()
{
    m_isDirty = false;
    delete m_viewProps;
    m_viewProps = 0;
}

void ViewPropertiesDialog::slotOk()
{
    applyViewProperties();
    accept();
}

void ViewPropertiesDialog::slotApply()
{
    applyViewProperties();
}

void ViewPropertiesDialog::slotViewModeChanged(int index)
{
    m_viewProps->setViewMode(static_cast<DolphinView::Mode>(index));
    m_isDirty = true;

    const bool iconsViewEnabled = (m_viewProps->viewMode() == DolphinView::IconsView);
    m_categorizedSorting->setEnabled(iconsViewEnabled);
    m_additionalInfo->setEnabled(iconsViewEnabled);
}

void ViewPropertiesDialog::slotSortingChanged(int index)
{
    const DolphinView::Sorting sorting = DolphinSortFilterProxyModel::sortingForColumn(index);
    m_viewProps->setSorting(sorting);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotSortOrderChanged(int index)
{
    Qt::SortOrder sortOrder = (index == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
    m_viewProps->setSortOrder(sortOrder);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotCategorizedSortingChanged(int index)
{
    m_viewProps->setCategorizedSorting(index == 1);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotAdditionalInfoChanged(int index)
{
    KFileItemDelegate::AdditionalInformation info = KFileItemDelegate::NoInformation;
    switch (index) {
    case 1:  info = KFileItemDelegate::FriendlyMimeType; break;
    case 2:  info = KFileItemDelegate::Size; break;
    case 3:  info = KFileItemDelegate::ModificationTime; break;
    default: break;
    }
    m_viewProps->setAdditionalInfo(info);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotShowPreviewChanged()
{
    const bool show = m_showPreview->isChecked();
    m_viewProps->setShowPreview(show);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotShowHiddenFilesChanged()
{
    const bool show = m_showHiddenFiles->isChecked();
    m_viewProps->setShowHiddenFiles(show);
    m_isDirty = true;
}

void ViewPropertiesDialog::markAsDirty()
{
    m_isDirty = true;
}

void ViewPropertiesDialog::applyViewProperties()
{
    const bool applyToSubFolders = m_isDirty &&
                                   (m_applyToSubFolders != 0) &&
                                   m_applyToSubFolders->isChecked();
    if (applyToSubFolders) {
        const QString text(i18n("The view properties of all sub folders will be changed. Do you want to continue?"));
        if (KMessageBox::questionYesNo(this, text) == KMessageBox::No) {
            return;
        }

        ViewPropsProgressInfo* info = new ViewPropsProgressInfo(m_dolphinView,
                                      m_dolphinView->url(),
                                      *m_viewProps);
        info->setWindowModality(Qt::NonModal);
        info->show();
    }

    const bool applyToAllFolders = m_isDirty &&
                                   (m_applyToAllFolders != 0) &&
                                   m_applyToAllFolders->isChecked();
    if (applyToAllFolders) {
        const QString text(i18n("The view properties of all folders will be changed. Do you want to continue?"));
        if (KMessageBox::questionYesNo(this, text) == KMessageBox::No) {
            return;
        }

        // Updating the global view properties time stamp in the general settings makes
        // all existing viewproperties invalid, as they have a smaller time stamp.
        GeneralSettings* settings = DolphinSettings::instance().generalSettings();
        settings->setViewPropsTimestamp(QDateTime::currentDateTime());

        // This is also a good chance to make a cleanup of all mirrored view properties:
        const KUrl mirroredDir = ViewProperties::mirroredDirectory();
        KIO::NetAccess::del(mirroredDir, this);
    }

    m_viewProps->save();

    m_dolphinView->setMode(m_viewProps->viewMode());
    m_dolphinView->setSorting(m_viewProps->sorting());
    m_dolphinView->setSortOrder(m_viewProps->sortOrder());
    m_dolphinView->setCategorizedSorting(m_viewProps->categorizedSorting());
    m_dolphinView->setAdditionalInfo(m_viewProps->additionalInfo());
    m_dolphinView->setShowPreview(m_viewProps->showPreview());
    m_dolphinView->setShowHiddenFiles(m_viewProps->showHiddenFiles());

    m_isDirty = false;

    if (m_useAsDefault->isChecked()) {
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
}

#include "viewpropertiesdialog.moc"
