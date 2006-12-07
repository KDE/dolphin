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
#include "viewpropsprogressinfo.h"

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

#include "viewproperties.h"
#include "dolphinview.h"

ViewPropertiesDialog::ViewPropertiesDialog(DolphinView* dolphinView) :
    KDialog(dolphinView),
    m_isDirty(false),
    m_dolphinView(dolphinView)
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
    m_viewMode->addItem(SmallIcon("gvdirpart"), i18n("Previews"));
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

    m_showHiddenFiles = new QCheckBox(i18n("Show hidden files"), propsBox);
    m_showHiddenFiles->setChecked(m_viewProps->isShowHiddenFilesEnabled());

    QGridLayout* propsBoxLayout = new QGridLayout(propsBox);
    propsBoxLayout->addWidget(viewModeLabel, 0, 0);
    propsBoxLayout->addWidget(m_viewMode, 0, 1);
    propsBoxLayout->addWidget(m_sorting, 1, 1);
    propsBoxLayout->addWidget(sortingLabel, 1, 0);
    propsBoxLayout->addWidget(m_sorting, 1, 1);
    propsBoxLayout->addWidget(sortOrderLabel, 2, 0);
    propsBoxLayout->addWidget(m_sortOrder, 2, 1);
    propsBoxLayout->addWidget(m_showHiddenFiles, 3, 0);

    // create 'Apply view properties to:' group
    QGroupBox* buttonBox = new QGroupBox(i18n("Apply view properties to:"), main);
    m_applyToCurrentFolder = new QRadioButton(i18n("Current folder"), buttonBox);
    m_applyToSubFolders = new QRadioButton(i18n("Current folder including all sub folders"), buttonBox);
    m_applyToAllFolders = new QRadioButton(i18n("All folders"), buttonBox);

    QVBoxLayout* buttonBoxLayout = new QVBoxLayout();
    buttonBoxLayout->addWidget(m_applyToCurrentFolder);
    buttonBoxLayout->addWidget(m_applyToSubFolders);
    buttonBoxLayout->addWidget(m_applyToAllFolders);
    buttonBox->setLayout(buttonBoxLayout);

    m_applyToCurrentFolder->setChecked(true);

    topLayout->addWidget(propsBox);
    topLayout->addWidget(buttonBox);

    connect(m_viewMode, SIGNAL(activated(int)),
            this, SLOT(slotViewModeChanged(int)));
    connect(m_sorting, SIGNAL(activated(int)),
            this, SLOT(slotSortingChanged(int)));
    connect(m_sortOrder, SIGNAL(activated(int)),
            this, SLOT(slotSortOrderChanged(int)));
    connect(m_showHiddenFiles, SIGNAL(clicked()),
            this, SLOT(slotShowHiddenFilesChanged()));
    connect(m_applyToCurrentFolder, SIGNAL(clicked()),
            this, SLOT(markAsDirty()));
    connect(m_applyToSubFolders, SIGNAL(clicked()),
            this, SLOT(markAsDirty()));
    connect(m_applyToAllFolders, SIGNAL(clicked()),
            this, SLOT(markAsDirty()));

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(slotApply()));

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

void ViewPropertiesDialog::slotShowHiddenFilesChanged()
{
    const bool show = m_showHiddenFiles->isChecked();
    m_viewProps->setShowHiddenFilesEnabled(show);
    m_isDirty = true;
}

void ViewPropertiesDialog::markAsDirty()
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

        //ViewProperties props(QDir::homePath());
        //props.setViewMode(m_viewProps->viewMode());
        //props.setSorting(m_viewProps->sorting());
        //props.setSortOrder(m_viewProps->sortOrder());
        //props.setShowHiddenFilesEnabled(m_viewProps->isShowHiddenFilesEnabled());
        //props.setValidForSubDirs(true);
    }
    else if (m_applyToSubFolders->isChecked() && m_isDirty) {
        const QString text(i18n("The view properties of all sub folders will be replaced. Do you want to continue?"));
        if (KMessageBox::questionYesNo(this, text) == KMessageBox::No) {
            return;
        }

        ViewPropsProgressInfo dlg(this, m_dolphinView->url(), m_viewProps);
        dlg.exec();
    }

    m_viewProps->save();
    m_dolphinView->setViewProperties(*m_viewProps);
    m_isDirty = false;
}

#include "viewpropertiesdialog.moc"
