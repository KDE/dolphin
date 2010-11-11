/***************************************************************************
*    Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>            *
*                                                                         *
*    This program is free software; you can redistribute it and/or modify *
*    it under the terms of the GNU General Public License as published by *
*    the Free Software Foundation; either version 2 of the License, or    *
*    (at your option) any later version.                                  *
*                                                                         *
*    This program is distributed in the hope that it will be useful,      *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*    GNU General Public License for more details.                         *
*                                                                         *
*    You should have received a copy of the GNU General Public License    *
*    along with this program; if not, write to the                        *
*    Free Software Foundation, Inc.,                                      *
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA           *
* **************************************************************************/

#include "dolphinsearchbox.h"

#include "dolphin_searchsettings.h"

#include <kicon.h>
#include <klineedit.h>
#include <klocale.h>
#include <kseparator.h>

#include <QButtonGroup>
#include <QDir>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #include <nepomuk/andterm.h>
    #include <nepomuk/filequery.h>
    #include <nepomuk/literalterm.h>
    #include <nepomuk/query.h>
    #include <nepomuk/queryparser.h>
    #include <nepomuk/resourcemanager.h>
    #include <nepomuk/resourcetypeterm.h>
    #include <nepomuk/comparisonterm.h>
    #include <nepomuk/nfo.h>
#endif

DolphinSearchBox::DolphinSearchBox(QWidget* parent) :
    QWidget(parent),
    m_startedSearching(false),
    m_nepomukActivated(false),
    m_topLayout(0),
    m_searchInput(0),
    m_fromHereButton(0),
    m_everywhereButton(0),
    m_fileNameButton(0),
    m_contentButton(0),
    m_searchPath(),
    m_startSearchTimer(0)
{
}

DolphinSearchBox::~DolphinSearchBox()
{
    saveSettings();
}

QString DolphinSearchBox::text() const
{
    return m_searchInput->text();
}

void DolphinSearchBox::setSearchPath(const KUrl& url)
{
    m_searchPath = url;

    QFontMetrics metrics(m_fromHereButton->font());
    const int maxWidth = metrics.averageCharWidth() * 15;
    const QString fileName = metrics.elidedText(url.fileName(), Qt::ElideMiddle, maxWidth);
    m_fromHereButton->setText(i18nc("action:button", "From Here (%1)", fileName));
}

KUrl DolphinSearchBox::searchPath() const
{
    return m_searchPath;
}

KUrl DolphinSearchBox::urlForSearching() const
{
    KUrl url;
    if (m_nepomukActivated && isSearchPathIndexed()) {
        url = nepomukUrlForSearching();
    } else {
        url = m_searchPath;
        url.setProtocol("filenamesearch");
        url.addQueryItem("search", m_searchInput->text());
        if (m_contentButton->isChecked()) {
            url.addQueryItem("checkContent", "yes");
        }
        if (m_everywhereButton->isChecked()) {
            // It is very unlikely, that the majority of Dolphins target users
            // mean "the whole harddisk" instead of "my home folder" when
            // selecting the "Everywhere" button.
            url.setPath(QDir::homePath());
        }
    }

    return url;
}

bool DolphinSearchBox::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        init();
    }
    return QWidget::event(event);
}

void DolphinSearchBox::showEvent(QShowEvent* event)
{
    if (!event->spontaneous()) {
#ifdef HAVE_NEPOMUK
        m_nepomukActivated = (Nepomuk::ResourceManager::instance()->init() == 0);
#endif

        m_searchInput->clear();
        m_searchInput->setFocus();
        m_startedSearching = false;
    }
}

void DolphinSearchBox::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);
    if ((event->key() == Qt::Key_Escape)) {
        if (m_searchInput->text().isEmpty()) {
            emit closeRequest();
        } else {
            m_searchInput->clear();
        }
    }
}

void DolphinSearchBox::emitSearchSignal()
{
    m_startSearchTimer->stop();
    m_startedSearching = true;
    emit search(m_searchInput->text());
}

void DolphinSearchBox::slotConfigurationChanged()
{
    if (m_startedSearching) {
        emitSearchSignal();
    }
}

void DolphinSearchBox::slotSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        m_startSearchTimer->stop();
    } else {
        m_startSearchTimer->start();
    }
    emit searchTextChanged(text);
}

void DolphinSearchBox::slotReturnPressed(const QString& text)
{
    emitSearchSignal();
    emit returnPressed(text);
}

void DolphinSearchBox::initButton(QPushButton* button)
{
    button->setAutoExclusive(true);
    button->setFlat(true);
    button->setCheckable(true);
    connect(button, SIGNAL(toggled(bool)), this, SLOT(slotConfigurationChanged()));
}

void DolphinSearchBox::loadSettings()
{
    if (SearchSettings::location() == QLatin1String("Everywhere")) {
        m_everywhereButton->setChecked(true);
    } else {
        m_fromHereButton->setChecked(true);
    }

    if (SearchSettings::what() == QLatin1String("Content")) {
        m_contentButton->setChecked(true);
    } else {
        m_fileNameButton->setChecked(true);
    }
}

void DolphinSearchBox::saveSettings()
{
    SearchSettings::setLocation(m_fromHereButton->isChecked() ? "FromHere" : "Everywhere");
    SearchSettings::setWhat(m_fileNameButton->isChecked() ? "FileName" : "Content");
    SearchSettings::self()->writeConfig();
}

void DolphinSearchBox::init()
{
    // Create close button
    QToolButton* closeButton = new QToolButton(this);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(KIcon("dialog-close"));
    closeButton->setToolTip(i18nc("@info:tooltip", "Quit searching"));
    connect(closeButton, SIGNAL(clicked()), SIGNAL(closeRequest()));

    // Create search label
    QLabel* searchLabel = new QLabel(i18nc("@label:textbox", "Find:"), this);

    // Create search box
    m_searchInput = new KLineEdit(this);
    m_searchInput->setClearButtonShown(true);
    m_searchInput->setFont(KGlobalSettings::generalFont());
    connect(m_searchInput, SIGNAL(returnPressed(QString)),
            this, SLOT(slotReturnPressed(QString)));
    connect(m_searchInput, SIGNAL(textChanged(QString)),
            this, SLOT(slotSearchTextChanged(QString)));

    // Apply layout for the search input
    QHBoxLayout* searchInputLayout = new QHBoxLayout();
    searchInputLayout->setMargin(0);
    searchInputLayout->addWidget(closeButton);
    searchInputLayout->addWidget(searchLabel);
    searchInputLayout->addWidget(m_searchInput);

    // Create "From Here" and "Everywhere"button
    m_fromHereButton = new QPushButton(this);
    m_fromHereButton->setText(i18nc("action:button", "From Here"));
    initButton(m_fromHereButton);

    m_everywhereButton = new QPushButton(this);
    m_everywhereButton->setText(i18nc("action:button", "Everywhere"));
    initButton(m_everywhereButton);

    QButtonGroup* searchLocationGroup = new QButtonGroup(this);
    searchLocationGroup->addButton(m_fromHereButton);
    searchLocationGroup->addButton(m_everywhereButton);

    // Create "Filename" and "Content" button
    m_fileNameButton = new QPushButton(this);
    m_fileNameButton->setText(i18nc("action:button", "Filename"));
    initButton(m_fileNameButton);

    m_contentButton = new QPushButton();
    m_contentButton->setText(i18nc("action:button", "Content"));
    initButton(m_contentButton);;

    QButtonGroup* searchWhatGroup = new QButtonGroup(this);
    searchWhatGroup->addButton(m_fileNameButton);
    searchWhatGroup->addButton(m_contentButton);

    // Apply layout for the options
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    optionsLayout->setMargin(0);
    optionsLayout->addWidget(m_fromHereButton);
    optionsLayout->addWidget(m_everywhereButton);
    optionsLayout->addWidget(new KSeparator(Qt::Vertical));
    optionsLayout->addWidget(m_fileNameButton);
    optionsLayout->addWidget(m_contentButton);
    optionsLayout->addStretch(1);

    m_topLayout = new QVBoxLayout(this);
    m_topLayout->addLayout(searchInputLayout);
    m_topLayout->addLayout(optionsLayout);

    searchLabel->setBuddy(m_searchInput);
    loadSettings();

    // The searching should be started automatically after the user did not change
    // the text within one second
    m_startSearchTimer = new QTimer(this);
    m_startSearchTimer->setSingleShot(true);
    m_startSearchTimer->setInterval(1000);
    connect(m_startSearchTimer, SIGNAL(timeout()), this, SLOT(emitSearchSignal()));
}

bool DolphinSearchBox::isSearchPathIndexed() const
{
#ifdef HAVE_NEPOMUK
    const QString path = m_searchPath.path();

    const KConfig strigiConfig("nepomukstrigirc");
    const QStringList indexedFolders = strigiConfig.group("General").readPathEntry("folders", QStringList());

    // Check whether the current search path is part of an indexed folder
    bool isIndexed = false;
    foreach (const QString& indexedFolder, indexedFolders) {
        if (path.startsWith(indexedFolder)) {
            isIndexed = true;
            break;
        }
    }

    if (isIndexed) {
        // The current search path is part of an indexed folder. Check whether no
        // excluded folder is part of the search path.
        const QStringList excludedFolders = strigiConfig.group("General").readPathEntry("exclude folders", QStringList());
        foreach (const QString& excludedFolder, excludedFolders) {
            if (path.startsWith(excludedFolder)) {
                isIndexed = false;
                break;
            }
        }
    }

    return isIndexed;
#else
    return false;
#endif
}

KUrl DolphinSearchBox::nepomukUrlForSearching() const
{
#ifdef HAVE_NEPOMUK
    Nepomuk::Query::AndTerm andTerm;

    // Add input from search filter
    const QString text = m_searchInput->text();
    if (!text.isEmpty()) {
        if (m_fileNameButton->isChecked()) {
            QString regex = QRegExp::escape(text);
            regex.replace("\\*", QLatin1String(".*"));
            regex.replace("\\?", QLatin1String("."));
            regex.replace("\\", "\\\\");
            andTerm.addSubTerm(Nepomuk::Query::ComparisonTerm(
                                    Nepomuk::Vocabulary::NFO::fileName(),
                                    Nepomuk::Query::LiteralTerm(regex),
                                    Nepomuk::Query::ComparisonTerm::Regexp));
        } else {
            const Nepomuk::Query::Query customQuery = Nepomuk::Query::QueryParser::parseQuery(text, Nepomuk::Query::QueryParser::DetectFilenamePattern);
            if (customQuery.isValid()) {
                andTerm.addSubTerm(customQuery.term());
            }
        }
    }

    Nepomuk::Query::FileQuery fileQuery;
    fileQuery.setFileMode(Nepomuk::Query::FileQuery::QueryFiles);
    fileQuery.setTerm(andTerm);

    return fileQuery.toSearchUrl(i18nc("@title UDS_DISPLAY_NAME for a KIO directory listing. %1 is the query the user entered.",
                                       "Query Results from '%1'",
                                       text));
#else
    return KUrl();
#endif
}

#include "dolphinsearchbox.moc"
