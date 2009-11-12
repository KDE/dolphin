/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "dolphinsearchoptionsconfigurator.h"

#include "searchcriterionselector.h"

#include <kcombobox.h>
#include <kdialog.h>
#include <kicon.h>
#include <klineedit.h>
#include <klocale.h>
#include <kseparator.h>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

DolphinSearchOptionsConfigurator::DolphinSearchOptionsConfigurator(QWidget* parent) :
    QWidget(parent),
    m_initialized(false),
    m_searchFromBox(0),
    m_searchWhatBox(0),
    m_addSelectorButton(0),
    m_vBoxLayout(0)
{
    m_vBoxLayout = new QVBoxLayout(this);

    // add "search" configuration
    QLabel* searchLabel = new QLabel(i18nc("@label", "Search:"));

    m_searchFromBox = new KComboBox(this);
    m_searchFromBox->addItem(i18nc("@label", "Everywhere"));
    m_searchFromBox->addItem(i18nc("@label", "From Here"));

    // add "what" configuration
    QLabel* whatLabel = new QLabel(i18nc("@label", "What:"));

    m_searchWhatBox = new KComboBox(this);
    m_searchWhatBox->addItem(i18nc("@label", "All"));
    m_searchWhatBox->addItem(i18nc("@label", "Images"));
    m_searchWhatBox->addItem(i18nc("@label", "Text"));
    m_searchWhatBox->addItem(i18nc("@label", "Filenames"));

    // add button "Save"
    QPushButton* saveButton = new QPushButton(this);
    saveButton->setIcon(KIcon("document-save"));
    saveButton->setText(i18nc("@action:button", "Save"));
    saveButton->setToolTip(i18nc("@info", "Save search options"));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveQuery()));

    // add button "Close"
    QPushButton* closeButton = new QPushButton(this);
    closeButton->setIcon(KIcon("dialog-close"));
    closeButton->setText(i18nc("@action:button", "Close"));
    closeButton->setToolTip(i18nc("@info", "Close search options"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(hide()));

    // add "Add selector" button
    m_addSelectorButton = new QPushButton(this);
    m_addSelectorButton->setIcon(KIcon("list-add"));
    m_addSelectorButton->setToolTip(i18nc("@info", "Add search option"));
    m_addSelectorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_addSelectorButton, SIGNAL(clicked()), this, SLOT(slotAddSelectorButtonClicked()));

    QHBoxLayout* firstLineLayout = new QHBoxLayout();
    firstLineLayout->addWidget(searchLabel);
    firstLineLayout->addWidget(m_searchFromBox);
    firstLineLayout->addWidget(whatLabel);
    firstLineLayout->addWidget(m_searchWhatBox);
    firstLineLayout->addWidget(new QWidget(this), 1); // filler
    firstLineLayout->addWidget(m_addSelectorButton);

    QHBoxLayout* lastLineLayout = new QHBoxLayout();
    lastLineLayout->addWidget(new QWidget(this), 1); // filler
    lastLineLayout->addWidget(saveButton);
    lastLineLayout->addWidget(closeButton);

    m_vBoxLayout->addWidget(new KSeparator(this));
    m_vBoxLayout->addLayout(firstLineLayout);
    m_vBoxLayout->addLayout(lastLineLayout);
    m_vBoxLayout->addWidget(new KSeparator(this));
}

DolphinSearchOptionsConfigurator::~DolphinSearchOptionsConfigurator()
{
}

void DolphinSearchOptionsConfigurator::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && !m_initialized) {
        // add default search criterions
        SearchCriterionSelector* dateCriterion = new SearchCriterionSelector(SearchCriterionSelector::Date, this);
        SearchCriterionSelector* sizeCriterion = new SearchCriterionSelector(SearchCriterionSelector::Size, this);
        SearchCriterionSelector* tagCriterion = new SearchCriterionSelector(SearchCriterionSelector::Tag, this);

        // Add the items in the same order as available in the description combo (verified by Q_ASSERTs). This
        // is not mandatory from an implementation point of view, but preferable from a usability point of view.
        Q_ASSERT(static_cast<int>(SearchCriterionSelector::Date) == 0);
        Q_ASSERT(static_cast<int>(SearchCriterionSelector::Size) == 1);
        Q_ASSERT(static_cast<int>(SearchCriterionSelector::Tag) == 2);
        addSelector(dateCriterion);
        addSelector(sizeCriterion);
        addSelector(tagCriterion);

        m_initialized = true;
    }
    QWidget::showEvent(event);
}

void DolphinSearchOptionsConfigurator::slotAddSelectorButtonClicked()
{
    SearchCriterionSelector* selector = new SearchCriterionSelector(SearchCriterionSelector::Tag, this);
    addSelector(selector);
}

void DolphinSearchOptionsConfigurator::removeCriterion()
{
    QWidget* criterion = qobject_cast<QWidget*>(sender());
    Q_ASSERT(criterion != 0);
    m_vBoxLayout->removeWidget(criterion);
    criterion->deleteLater();

    updateSelectorButton();
}

void DolphinSearchOptionsConfigurator::updateSelectorButton()
{
    const int selectors = m_vBoxLayout->count() - 1;
    m_addSelectorButton->setEnabled(selectors < 10);
}

void DolphinSearchOptionsConfigurator::saveQuery()
{
    KDialog dialog(0, Qt::Dialog);

    QWidget* container = new QWidget(&dialog);

    QLabel* label = new QLabel(i18nc("@label", "Name:"), container);
    KLineEdit* lineEdit = new KLineEdit(container);
    lineEdit->setMinimumWidth(250);

    QHBoxLayout* layout = new QHBoxLayout(container);
    layout->addWidget(label, Qt::AlignRight);
    layout->addWidget(lineEdit);

    dialog.setMainWidget(container);
    dialog.setCaption(i18nc("@title:window", "Save Search Options"));
    dialog.setButtons(KDialog::Ok | KDialog::Cancel);
    dialog.setDefaultButton(KDialog::Ok);
    dialog.setButtonText(KDialog::Ok, i18nc("@action:button", "Save"));

    KConfigGroup dialogConfig(KSharedConfig::openConfig("dolphinrc"),
                              "SaveSearchOptionsDialog");
    dialog.restoreDialogSize(dialogConfig);
    dialog.exec(); // TODO...
}

void DolphinSearchOptionsConfigurator::addSelector(SearchCriterionSelector* selector)
{
    connect(selector, SIGNAL(removeCriterion()), this, SLOT(removeCriterion()));

    // insert the new selector before the lastLineLayout and the KSeparator at the bottom
    const int index = m_vBoxLayout->count() - 2;
    m_vBoxLayout->insertWidget(index, selector);
    updateSelectorButton();
}

#include "dolphinsearchoptionsconfigurator.moc"
