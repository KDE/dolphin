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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "viewpropertiesdialog.h"
#include <klocale.h>
#include <qlabel.h>
#include <qlayout.h>
#include <q3vbox.h>
#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <q3groupbox.h>
#include <qcombobox.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <assert.h>

#include "viewproperties.h"
#include "dolphinview.h"

ViewPropertiesDialog::ViewPropertiesDialog(DolphinView* dolphinView) :
    KDialog(dolphinView, static_cast<Qt::WFlags>(Ok /* KDE4-TODO: Apply | Cancel*/)),
    m_isDirty(false),
    m_dolphinView(dolphinView)
{
    assert(dolphinView != 0);

    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    const KUrl& url = dolphinView->url();
    m_viewProps = new ViewProperties(url);
    m_viewProps->setAutoSaveEnabled(false);

    Q3VBoxLayout* topLayout = new Q3VBoxLayout(mainWidget(), 0, spacingHint());

    // create 'Properties' group containing view mode, sorting, sort order and show hidden files
    Q3GroupBox* propsGroup = new Q3GroupBox(2, Qt::Horizontal, i18n("Properties"), mainWidget());
    propsGroup->setSizePolicy(sizePolicy);
    propsGroup->setMargin(margin);

    new QLabel(i18n("View mode:"), propsGroup);
    m_viewMode = new QComboBox(propsGroup);
    m_viewMode->insertItem(SmallIcon("view_icon"), i18n("Icons"));
    m_viewMode->insertItem(SmallIcon("view_text"), i18n("Details"));
    m_viewMode->insertItem(SmallIcon("gvdirpart"), i18n("Previews"));
    const int index = static_cast<int>(m_viewProps->viewMode());
    m_viewMode->setCurrentItem(index);

    new QLabel(i18n("Sorting:"), propsGroup);
    m_sorting = new QComboBox(propsGroup);
    m_sorting->insertItem("By Name");
    m_sorting->insertItem("By Size");
    m_sorting->insertItem("By Date");
    int sortingIdx = 0;
    switch (m_viewProps->sorting()) {
        case DolphinView::SortByName: sortingIdx = 0; break;
        case DolphinView::SortBySize: sortingIdx = 1; break;
        case DolphinView::SortByDate: sortingIdx = 2; break;
        default: break;
    }
    m_sorting->setCurrentItem(sortingIdx);

    new QLabel(i18n("Sort order:"), propsGroup);
    m_sortOrder = new QComboBox(propsGroup);
    m_sortOrder->insertItem(i18n("Ascending"));
    m_sortOrder->insertItem(i18n("Descending"));
    const int sortOrderIdx = (m_viewProps->sortOrder() == Qt::Ascending) ? 0 : 1;
    m_sortOrder->setCurrentItem(sortOrderIdx);

    m_showHiddenFiles = new QCheckBox(i18n("Show hidden files"), propsGroup);
    m_showHiddenFiles->setChecked(m_viewProps->isShowHiddenFilesEnabled());

    // create 'Apply view properties to:' group
    Q3ButtonGroup* buttonGroup = new Q3ButtonGroup(3,
                                                 Qt::Vertical,
                                                 i18n("Apply view properties to:"),
                                                 mainWidget());
    buttonGroup->setSizePolicy(sizePolicy);
    buttonGroup->setMargin(margin);

    m_applyToCurrentFolder = new QRadioButton(i18n("Current folder"), buttonGroup);
    buttonGroup->insert(m_applyToCurrentFolder);

    m_applyToSubFolders = new QRadioButton(i18n("Current folder including all sub folders"), buttonGroup);
    buttonGroup->insert(m_applyToSubFolders);

    m_applyToAllFolders = new QRadioButton(i18n("All folders"), buttonGroup);
    buttonGroup->insert(m_applyToAllFolders);

    if (m_viewProps->isValidForSubDirs()) {
        m_applyToSubFolders->setChecked(true);
    }
    else {
        m_applyToCurrentFolder->setChecked(true);
    }

    topLayout->addWidget(propsGroup);
    topLayout->addWidget(buttonGroup);

    connect(m_viewMode, SIGNAL(activated(int)),
            this, SLOT(slotViewModeChanged(int)));
    connect(m_sorting, SIGNAL(activated(int)),
            this, SLOT(slotSortingChanged(int)));
    connect(m_sortOrder, SIGNAL(activated(int)),
            this, SLOT(slotSortOrderChanged(int)));
    connect(m_showHiddenFiles, SIGNAL(clicked()),
            this, SLOT(slotShowHiddenFilesChanged()));
    connect(m_applyToCurrentFolder, SIGNAL(clicked()),
            this, SLOT(slotApplyToCurrentFolder()));
    connect(m_applyToSubFolders, SIGNAL(clicked()),
            this, SLOT(slotApplyToSubFolders()));
    connect(m_applyToAllFolders, SIGNAL(clicked()),
            this, SLOT(slotApplyToAllFolders()));
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
    // KDE4-TODO: KDialog::slotOk();
}

void ViewPropertiesDialog::slotApply()
{
    applyViewProperties();
    // KDE4-TODO: KDialog::slotApply();
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

void ViewPropertiesDialog::slotShowHiddenFilesChanged()
{
    const bool show = m_showHiddenFiles->isChecked();
    m_viewProps->setShowHiddenFilesEnabled(show);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotApplyToCurrentFolder()
{
    m_viewProps->setValidForSubDirs(false);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotApplyToSubFolders()
{
    m_viewProps->setValidForSubDirs(true);
    m_isDirty = true;
}

void ViewPropertiesDialog::slotApplyToAllFolders()
{
    m_isDirty = true;
}

void ViewPropertiesDialog::applyViewProperties()
{
    if (m_applyToAllFolders->isChecked()) {
        if (m_isDirty) {
            const QString text(i18n("The view properties of all folders will be replaced. Do you want to continue?"));
            if (KMessageBox::questionYesNo(this, text) == KMessageBox::No) {
                return;
            }
        }

        ViewProperties props(QDir::homePath());
        props.setViewMode(m_viewProps->viewMode());
        props.setSorting(m_viewProps->sorting());
        props.setSortOrder(m_viewProps->sortOrder());
        props.setShowHiddenFilesEnabled(m_viewProps->isShowHiddenFilesEnabled());
        props.setValidForSubDirs(true);
    }
    else if (m_applyToSubFolders->isChecked() && m_isDirty) {
        const QString text(i18n("The view properties of all sub folders will be replaced. Do you want to continue?"));
        if (KMessageBox::questionYesNo(this, text) == KMessageBox::No) {
            return;
        }
    }

    m_viewProps->save();
    m_dolphinView->setViewProperties(*m_viewProps);
    m_isDirty = false;
}

#include "viewpropertiesdialog.moc"
