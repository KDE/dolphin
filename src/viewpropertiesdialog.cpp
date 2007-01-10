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
#include "generalsettings.h"
#include "viewproperties.h"

#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <assert.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

ViewPropertiesDialog::ViewPropertiesDialog(DolphinView* dolphinView) :
    KDialog(dolphinView),
    m_isDirty(false),
    m_dolphinView(dolphinView),
    m_viewProps(0),
    m_viewMode(0),
    m_sorting(0),
    m_sortOrder(0),
    m_showPreview(0),
    m_showHiddenFiles(0),
    m_applyToSubFolders(0),
    m_useAsDefault(0)
{
    assert(dolphinView != 0);

    setCaption(i18n("View Properties"));
    setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);

    const KUrl& url = dolphinView->url();
    m_viewProps = new ViewProperties(url);
    m_viewProps->setAutoSaveEnabled(false);

    QWidget* main = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout();

    // create 'Properties' group containing view mode, sorting, sort order and show hidden files
    QGroupBox* propsBox = new QGroupBox(i18n("Properties"), main);

    QLabel* viewModeLabel = new QLabel(i18n("View mode:"), propsBox);
    m_viewMode = new QComboBox(propsBox);
    m_viewMode->addItem(SmallIcon("view_icon"), i18n("Icons"));
    m_viewMode->addItem(SmallIcon("view_text"), i18n("Details"));
    const int index = static_cast<int>(m_viewProps->viewMode());
    m_viewMode->setCurrentIndex(index);

    QLabel* sortingLabel = new QLabel(i18n("Sorting:"), propsBox);
    m_sorting = new QComboBox(propsBox);
    m_sorting->addItem("By Name");
    m_sorting->addItem("By Size");
    m_sorting->addItem("By Date");
    int sortingIdx = 0;
    switch (m_viewProps->sorting()) {
        case DolphinView::SortByName: sortingIdx = 0; break;
        case DolphinView::SortBySize: sortingIdx = 1; break;
        case DolphinView::SortByDate: sortingIdx = 2; break;
        default: break;
    }
    m_sorting->setCurrentIndex(sortingIdx);

    QLabel* sortOrderLabel = new QLabel(i18n("Sort order:"), propsBox);
    m_sortOrder = new QComboBox(propsBox);
    m_sortOrder->addItem(i18n("Ascending"));
    m_sortOrder->addItem(i18n("Descending"));
    const int sortOrderIdx = (m_viewProps->sortOrder() == Qt::Ascending) ? 0 : 1;
    m_sortOrder->setCurrentIndex(sortOrderIdx);

    m_showPreview = new QCheckBox(i18n("Show preview"), propsBox);
    m_showPreview->setChecked(m_viewProps->showPreview());

    m_showHiddenFiles = new QCheckBox(i18n("Show hidden files"), propsBox);
    m_showHiddenFiles->setChecked(m_viewProps->showHiddenFiles());

    QGridLayout* propsBoxLayout = new QGridLayout(propsBox);
    propsBoxLayout->addWidget(viewModeLabel, 0, 0);
    propsBoxLayout->addWidget(m_viewMode, 0, 1);
    propsBoxLayout->addWidget(m_sorting, 1, 1);
    propsBoxLayout->addWidget(sortingLabel, 1, 0);
    propsBoxLayout->addWidget(m_sorting, 1, 1);
    propsBoxLayout->addWidget(sortOrderLabel, 2, 0);
    propsBoxLayout->addWidget(m_sortOrder, 2, 1);
    propsBoxLayout->addWidget(m_showPreview, 3, 0);
    propsBoxLayout->addWidget(m_showHiddenFiles, 4, 0);

    topLayout->addWidget(propsBox);

    connect(m_viewMode, SIGNAL(activated(int)),
            this, SLOT(slotViewModeChanged(int)));
    connect(m_sorting, SIGNAL(activated(int)),
            this, SLOT(slotSortingChanged(int)));
    connect(m_sortOrder, SIGNAL(activated(int)),
            this, SLOT(slotSortOrderChanged(int)));
    connect(m_showPreview, SIGNAL(clicked()),
            this, SLOT(slotShowPreviewChanged()));
    connect(m_showHiddenFiles, SIGNAL(clicked()),
            this, SLOT(slotShowHiddenFilesChanged()));

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));

    // Only show the following settings if the view properties are remembered
    // for each directory:
    if (!DolphinSettings::instance().generalSettings()->globalViewProps()) {
        m_applyToSubFolders = new QCheckBox(i18n("Apply changes to all sub folders"), main);
        m_useAsDefault = new QCheckBox(i18n("Use as default"), main);
        topLayout->addWidget(m_applyToSubFolders);
        topLayout->addWidget(m_useAsDefault);

        connect(m_applyToSubFolders, SIGNAL(clicked()),
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
    assert((index >= 0) && (index <= 2));
    m_viewProps->setViewMode(static_cast<DolphinView::Mode>(index));
    m_isDirty = true;
}

void ViewPropertiesDialog::slotSortingChanged(int index)
{
    DolphinView::Sorting sorting = DolphinView::SortByName;
    switch (index) {
        case 1: sorting = DolphinView::SortBySize; break;
        case 2: sorting = DolphinView::SortByDate; break;
        default: break;
    }
    m_viewProps->setSorting(sorting);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotSortOrderChanged(int index)
{
    Qt::SortOrder sortOrder = (index == 0) ? Qt::Ascending : Qt::Descending;
    m_viewProps->setSortOrder(sortOrder);
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

    m_viewProps->save();

    m_dolphinView->setMode(m_viewProps->viewMode());
    m_dolphinView->setSorting(m_viewProps->sorting());
    m_dolphinView->setSortOrder(m_viewProps->sortOrder());
    m_dolphinView->setShowPreview(m_viewProps->showPreview());
    m_dolphinView->setShowHiddenFiles(m_viewProps->showHiddenFiles());

    m_isDirty = false;

    // TODO: handle m_useAsDefault setting
}

#include "viewpropertiesdialog.moc"
