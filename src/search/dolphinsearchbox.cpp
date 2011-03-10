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
#include "dolphinsearchinformation.h"

#include <KIcon>
#include <KLineEdit>
#include <KLocale>
#include <KSeparator>

#include <QButtonGroup>
#include <QDir>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #include <Nepomuk/Query/FileQuery>
    #include <Nepomuk/Query/LiteralTerm>
    #include <Nepomuk/Query/OrTerm>
    #include <Nepomuk/Query/Query>
    #include <Nepomuk/Query/QueryParser>
    #include <Nepomuk/Query/ResourceTypeTerm>
    #include <Nepomuk/Query/ComparisonTerm>
    #include <Nepomuk/ResourceManager>
    #include <Nepomuk/Vocabulary/NFO>
#endif

DolphinSearchBox::DolphinSearchBox(QWidget* parent) :
    QWidget(parent),
    m_startedSearching(false),
    m_readOnly(false),
    m_topLayout(0),
    m_searchInput(0),
    m_fileNameButton(0),
    m_contentButton(0),
    m_separator(0),
    m_fromHereButton(0),
    m_everywhereButton(0),
    m_infoLabel(0),
    m_searchPath(),
    m_startSearchTimer(0)
{
}

DolphinSearchBox::~DolphinSearchBox()
{
    saveSettings();
}

void DolphinSearchBox::setText(const QString& text)
{
    m_searchInput->setText(text);
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

    QString location = url.fileName();
    if (location.isEmpty()) {
        if (url.isLocalFile()) {
            location = QLatin1String("/");
        } else {
            location = url.protocol() + QLatin1String(" - ") + url.host();
        }
    }

    const QString elidedLocation = metrics.elidedText(location, Qt::ElideMiddle, maxWidth);
    m_fromHereButton->setText(i18nc("action:button", "From Here (%1)", elidedLocation));

    const bool showSearchFromButtons = url.isLocalFile() && !m_readOnly;
    m_separator->setVisible(showSearchFromButtons);
    m_fromHereButton->setVisible(showSearchFromButtons);
    m_everywhereButton->setVisible(showSearchFromButtons);
}

KUrl DolphinSearchBox::searchPath() const
{
    return m_searchPath;
}

KUrl DolphinSearchBox::urlForSearching() const
{
    KUrl url;
    const DolphinSearchInformation& searchInfo = DolphinSearchInformation::instance();
    if (searchInfo.isIndexingEnabled() && searchInfo.isPathIndexed(m_searchPath)) {
        url = nepomukUrlForSearching();
    } else {
        url.setProtocol("filenamesearch");
        url.addQueryItem("search", m_searchInput->text());
        if (m_contentButton->isChecked()) {
            url.addQueryItem("checkContent", "yes");
        }

        QString encodedUrl;
        if (m_everywhereButton->isChecked()) {
            // It is very unlikely, that the majority of Dolphins target users
            // mean "the whole harddisk" instead of "my home folder" when
            // selecting the "Everywhere" button.
            encodedUrl = QDir::homePath();
        } else {
            encodedUrl = m_searchPath.url();
        }
        url.addQueryItem("url", encodedUrl);
    }

    return url;
}

void DolphinSearchBox::selectAll()
{
    m_searchInput->selectAll();
}

void DolphinSearchBox::setReadOnly(bool readOnly)
{
    if (m_readOnly != readOnly) {
        m_readOnly = readOnly;
        applyReadOnlyState();
    }
}

bool DolphinSearchBox::isReadOnly() const
{
    return m_readOnly;
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
        m_searchInput->setFocus();
        m_startedSearching = false;
    }
}

void DolphinSearchBox::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Escape) {
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
    m_startSearchTimer->start();
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
    setFocusProxy(m_searchInput);
    connect(m_searchInput, SIGNAL(returnPressed(QString)),
            this, SLOT(slotReturnPressed(QString)));
    connect(m_searchInput, SIGNAL(textChanged(QString)),
            this, SLOT(slotSearchTextChanged(QString)));

    // Create information label
    m_infoLabel = new QLabel("TODO: Provide information about the current query", this);

    // Apply layout for the search input
    QHBoxLayout* searchInputLayout = new QHBoxLayout();
    searchInputLayout->setMargin(0);
    searchInputLayout->addWidget(closeButton);
    searchInputLayout->addWidget(searchLabel);
    searchInputLayout->addWidget(m_searchInput);
    searchInputLayout->addWidget(m_infoLabel);

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

    m_separator = new KSeparator(Qt::Vertical, this);

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

    // Apply layout for the options
    QHBoxLayout* optionsLayout = new QHBoxLayout();
    optionsLayout->setMargin(0);
    optionsLayout->addWidget(m_fileNameButton);
    optionsLayout->addWidget(m_contentButton);
    optionsLayout->addWidget(m_separator);
    optionsLayout->addWidget(m_fromHereButton);
    optionsLayout->addWidget(m_everywhereButton);
    optionsLayout->addStretch(1);

    // Put the options into a QScrollArea. This prevents increasing the view width
    // in case that not enough width for the options is available.
    QWidget* optionsContainer = new QWidget(this);
    optionsContainer->setLayout(optionsLayout);
    QScrollArea* optionsScrollArea = new QScrollArea(this);
    optionsScrollArea->setFrameShape(QFrame::NoFrame);
    optionsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    optionsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    optionsScrollArea->setMaximumHeight(optionsContainer->sizeHint().height());
    optionsScrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    optionsScrollArea->setWidget(optionsContainer);
    optionsScrollArea->setWidgetResizable(true);

    m_topLayout = new QVBoxLayout(this);
    m_topLayout->addLayout(searchInputLayout);
    m_topLayout->addWidget(optionsScrollArea);

    searchLabel->setBuddy(m_searchInput);
    loadSettings();

    // The searching should be started automatically after the user did not change
    // the text within one second
    m_startSearchTimer = new QTimer(this);
    m_startSearchTimer->setSingleShot(true);
    m_startSearchTimer->setInterval(1000);
    connect(m_startSearchTimer, SIGNAL(timeout()), this, SLOT(emitSearchSignal()));

    applyReadOnlyState();
}

KUrl DolphinSearchBox::nepomukUrlForSearching() const
{
#ifdef HAVE_NEPOMUK
    Nepomuk::Query::OrTerm orTerm;

    const QString text = m_searchInput->text();

    // Search the text in the filename in any case
    QString regex = QRegExp::escape(text);
    regex.replace("\\*", QLatin1String(".*"));
    regex.replace("\\?", QLatin1String("."));
    regex.replace("\\", "\\\\");
    orTerm.addSubTerm(Nepomuk::Query::ComparisonTerm(
                            Nepomuk::Vocabulary::NFO::fileName(),
                            Nepomuk::Query::LiteralTerm(regex),
                            Nepomuk::Query::ComparisonTerm::Regexp));

    if (m_contentButton->isChecked()) {
        // Search the text also in the content of the files
        const Nepomuk::Query::Query customQuery = Nepomuk::Query::QueryParser::parseQuery(text, Nepomuk::Query::QueryParser::DetectFilenamePattern);
        if (customQuery.isValid()) {
            orTerm.addSubTerm(customQuery.term());
        }
    }

    Nepomuk::Query::FileQuery fileQuery;
    fileQuery.setFileMode(Nepomuk::Query::FileQuery::QueryFilesAndFolders);
    fileQuery.setTerm(orTerm);
    if (m_fromHereButton->isChecked()) {
        const bool recursive = true;
        fileQuery.addIncludeFolder(m_searchPath, recursive);
    }

    return fileQuery.toSearchUrl(i18nc("@title UDS_DISPLAY_NAME for a KIO directory listing. %1 is the query the user entered.",
                                       "Query Results from '%1'",
                                       text));
#else
    return KUrl();
#endif
}

void DolphinSearchBox::applyReadOnlyState()
{
    // TODO: This is just an early draft to indicate that a state change
    // has been done
    m_searchInput->setVisible(!m_readOnly);
    m_infoLabel->setVisible(m_readOnly);
    m_fileNameButton->setVisible(!m_readOnly);
    m_contentButton->setVisible(!m_readOnly);
    m_fromHereButton->setVisible(!m_readOnly);
    m_everywhereButton->setVisible(!m_readOnly);
}

#include "dolphinsearchbox.moc"
