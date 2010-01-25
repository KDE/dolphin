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

#include "dolphin_searchsettings.h"
#include <settings/dolphinsettings.h>

#define DISABLE_NEPOMUK_LEGACY
#include <nepomuk/andterm.h>
#include <nepomuk/filequery.h>
#include <nepomuk/orterm.h>
#include <nepomuk/queryparser.h>
#include <nepomuk/resourcetypeterm.h>
#include <nepomuk/term.h>

#include "nfo.h"

#include <kcombobox.h>
#include <kdialog.h>
#include <kfileplacesmodel.h>
#include <kicon.h>
#include <klineedit.h>
#include <klocale.h>
#include <kseparator.h>

#include "searchcriterionselector.h"
#include "searchoptiondialogbox.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

struct SettingsItem
{
    const char* settingsName;
    const char* text;
};

// Contains the settings names and translated texts
// for each item of the location-combo-box.
static const SettingsItem g_locationItems[] = {
    {"Everywhere", I18N_NOOP2("@label", "Everywhere")},
    {"From Here",  I18N_NOOP2("@label", "From Here")}
};

// Contains the settings names and translated texts
// for each item of the what-combobox.
static const SettingsItem g_whatItems[] = {
    {"All",       I18N_NOOP2("@label", "All")},
    {"Images",    I18N_NOOP2("@label", "Images")},
    {"Text",      I18N_NOOP2("@label", "Text")},
    {"Filenames", I18N_NOOP2("@label", "Filenames")}
};

struct CriterionItem
{
    const char* settingsName;
    SearchCriterionSelector::Type type;
};

// Contains the settings names for type
// of availabe search criterion.
static const CriterionItem g_criterionItems[] = {
    {"Date", SearchCriterionSelector::Date},
    {"Size", SearchCriterionSelector::Size},
    {"Tag", SearchCriterionSelector::Tag},
    {"Raging", SearchCriterionSelector::Rating}
};

DolphinSearchOptionsConfigurator::DolphinSearchOptionsConfigurator(QWidget* parent) :
    QWidget(parent),
    m_initialized(false),
    m_directory(),
    m_locationBox(0),
    m_whatBox(0),
    m_addSelectorButton(0),
    m_searchButton(0),
    m_saveButton(0),
    m_vBoxLayout(0),
    m_criteria(),
    m_customSearchQuery()
{
    m_vBoxLayout = new QVBoxLayout(this);

    // add "search" configuration
    QLabel* searchLabel = new QLabel(i18nc("@label", "Search:"));

    m_locationBox = new KComboBox(this);
    for (unsigned int i = 0; i < sizeof(g_locationItems) / sizeof(SettingsItem); ++i) {
        m_locationBox->addItem(g_locationItems[i].text);
    }

    // add "what" configuration
    QLabel* whatLabel = new QLabel(i18nc("@label", "What:"));

    m_whatBox = new KComboBox(this);
    for (unsigned int i = 0; i < sizeof(g_whatItems) / sizeof(SettingsItem); ++i) {
        m_whatBox->addItem(g_whatItems[i].text);
    }
    connect(m_whatBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateButtons()));

    // add "Add selector" button
    m_addSelectorButton = new QPushButton(this);
    m_addSelectorButton->setIcon(KIcon("list-add"));
    m_addSelectorButton->setToolTip(i18nc("@info", "Add search option"));
    m_addSelectorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_addSelectorButton, SIGNAL(clicked()), this, SLOT(slotAddSelectorButtonClicked()));

    // add button "Search"
    m_searchButton = new QPushButton(this);
    m_searchButton->setIcon(KIcon("edit-find"));
    m_searchButton->setText(i18nc("@action:button", "Search"));
    m_searchButton->setToolTip(i18nc("@info", "Start searching"));
    m_searchButton->setEnabled(false);
    connect(m_searchButton, SIGNAL(clicked()), this, SIGNAL(searchOptionsChanged()));

    // add button "Save"
    m_saveButton = new QPushButton(this);
    m_saveButton->setIcon(KIcon("document-save"));
    m_saveButton->setText(i18nc("@action:button", "Save"));
    m_saveButton->setToolTip(i18nc("@info", "Save search options"));
    m_saveButton->setEnabled(false);
    connect(m_saveButton, SIGNAL(clicked()), this, SLOT(saveQuery()));

    // add button "Close"
    QPushButton* closeButton = new QPushButton(this);
    closeButton->setIcon(KIcon("dialog-close"));
    closeButton->setText(i18nc("@action:button", "Close"));
    closeButton->setToolTip(i18nc("@info", "Close search options"));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(hide()));

    QHBoxLayout* topLineLayout = new QHBoxLayout();
    topLineLayout->addWidget(m_addSelectorButton);
    topLineLayout->addWidget(searchLabel);
    topLineLayout->addWidget(m_locationBox);
    topLineLayout->addWidget(whatLabel);
    topLineLayout->addWidget(m_whatBox);
    topLineLayout->addWidget(new QWidget(this), 1); // filler
    topLineLayout->addWidget(m_searchButton);
    topLineLayout->addWidget(m_saveButton);
    topLineLayout->addWidget(closeButton);

    m_vBoxLayout->addWidget(new KSeparator(this));
    m_vBoxLayout->addLayout(topLineLayout);
    m_vBoxLayout->addWidget(new KSeparator(this));
}

DolphinSearchOptionsConfigurator::~DolphinSearchOptionsConfigurator()
{
    // store the UI configuration
    const int locationIndex = m_locationBox->currentIndex();
    SearchSettings::setLocation(g_locationItems[locationIndex].settingsName);

    const int whatIndex = m_whatBox->currentIndex();
    SearchSettings::setWhat(g_whatItems[whatIndex].settingsName);

    QString criteriaString;
    foreach(const SearchCriterionSelector* criterion, m_criteria) {
        if (!criteriaString.isEmpty()) {
            criteriaString += ',';
        }
        const int index = static_cast<int>(criterion->type());
        criteriaString += g_criterionItems[index].settingsName;
    }
    SearchSettings::setCriteria(criteriaString);

    SearchSettings::self()->writeConfig();
}

QString DolphinSearchOptionsConfigurator::customSearchQuery() const
{
    return m_customSearchQuery;
}


KUrl DolphinSearchOptionsConfigurator::directory() const
{
    return m_directory;
}

KUrl DolphinSearchOptionsConfigurator::nepomukSearchUrl() const
{
    const Nepomuk::Query::Query query = nepomukQuery();
    return query.isValid() ? query.toSearchUrl() : KUrl();
}

void DolphinSearchOptionsConfigurator::setCustomSearchQuery(const QString& searchQuery)
{
    m_customSearchQuery = searchQuery.simplified();
    updateButtons();
}

void DolphinSearchOptionsConfigurator::setDirectory(const KUrl& dir)
{
    if (dir.protocol() != QString::fromLatin1("nepomuksearch")) {
        m_directory = dir;
    }
}

void DolphinSearchOptionsConfigurator::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && !m_initialized) {
        // restore the UI layout of the last session
        const QString location = SearchSettings::location();
        for (unsigned int i = 0; i < sizeof(g_locationItems) / sizeof(SettingsItem); ++i) {
            if (g_locationItems[i].settingsName == location) {
                m_locationBox->setCurrentIndex(i);
                break;
            }
        }

        const QString what = SearchSettings::what();
        for (unsigned int i = 0; i < sizeof(g_whatItems) / sizeof(SettingsItem); ++i) {
            if (g_whatItems[i].settingsName == what) {
                m_whatBox->setCurrentIndex(i);
                break;
            }
        }

        const QString criteria = SearchSettings::criteria();
        QStringList criteriaList = criteria.split(',');
        foreach (const QString& criterionName, criteriaList) {
            for (unsigned int i = 0; i < sizeof(g_criterionItems) / sizeof(CriterionItem); ++i) {
                if (g_criterionItems[i].settingsName == criterionName) {
                    const SearchCriterionSelector::Type type = g_criterionItems[i].type;
                    addCriterion(new SearchCriterionSelector(type, this));
                    break;
                }
            }
        }

        m_initialized = true;
    }
    QWidget::showEvent(event);
}

void DolphinSearchOptionsConfigurator::slotAddSelectorButtonClicked()
{
    SearchCriterionSelector* selector = new SearchCriterionSelector(SearchCriterionSelector::Date, this);
    addCriterion(selector);
}

void DolphinSearchOptionsConfigurator::removeCriterion()
{
    SearchCriterionSelector* criterion = qobject_cast<SearchCriterionSelector*>(sender());
    Q_ASSERT(criterion != 0);
    m_vBoxLayout->removeWidget(criterion);

    const int index = m_criteria.indexOf(criterion);
    m_criteria.removeAt(index);

    criterion->deleteLater();

    updateButtons();
}

void DolphinSearchOptionsConfigurator::saveQuery()
{
    QPointer<SearchOptionDialogBox> dialog = new SearchOptionDialogBox( 0 );

    if (dialog->exec() == QDialog::Accepted) {
        KFilePlacesModel* model = DolphinSettings::instance().placesModel();
        model->addPlace(dialog->text(), nepomukSearchUrl());
    }
    delete dialog;
}

void DolphinSearchOptionsConfigurator::updateButtons()
{
    const bool enable = nepomukQuery().isValid();
    m_searchButton->setEnabled(enable);
    m_saveButton->setEnabled(enable);

    const int selectors = m_vBoxLayout->count() - 1;
    m_addSelectorButton->setEnabled(selectors < 10);
}

void DolphinSearchOptionsConfigurator::addCriterion(SearchCriterionSelector* criterion)
{
    connect(criterion, SIGNAL(removeCriterion()), this, SLOT(removeCriterion()));
    connect(criterion, SIGNAL(criterionChanged()), this, SLOT(updateButtons()));

    // insert the new selector before the KSeparator at the bottom
    const int index = m_vBoxLayout->count() - 1;
    m_vBoxLayout->insertWidget(index, criterion);
    updateButtons();

    m_criteria.append(criterion);
}

Nepomuk::Query::Query DolphinSearchOptionsConfigurator::nepomukQuery() const
{
    Nepomuk::Query::AndTerm andTerm;

    // add search criterion terms
    foreach (const SearchCriterionSelector* criterion, m_criteria) {
        const Nepomuk::Query::Term term = criterion->queryTerm();
        andTerm.addSubTerm(term);
    }

    // add custom query term from the searchbar
    const Nepomuk::Query::Query customQuery = Nepomuk::Query::QueryParser::parseQuery(m_customSearchQuery);
    if (customQuery.isValid()) {
        andTerm.addSubTerm(customQuery.term());
    }

    // filter result by the "What" filter
    switch (m_whatBox->currentIndex()) {
    case 1: {
        // Image
        const Nepomuk::Query::ResourceTypeTerm image(Nepomuk::Vocabulary::NFO::Image());
        andTerm.addSubTerm(image);
        break;
    }
    case 2: {
        // Text
        const Nepomuk::Query::ResourceTypeTerm textDocument(Nepomuk::Vocabulary::NFO::TextDocument());
        andTerm.addSubTerm(textDocument);
        break;
    }
    default: break;
    }

    Nepomuk::Query::FileQuery fileQuery;
    if ((m_locationBox->currentIndex() == 1) && m_directory.isValid()) {
        // "From Here" is selected as location filter
        fileQuery.addIncludeFolder(m_directory);
    }
    fileQuery.setTerm(andTerm);
    return fileQuery;
}

#include "dolphinsearchoptionsconfigurator.moc"
