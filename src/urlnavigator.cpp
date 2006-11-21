/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
 *   Copyright (C) 2006 by Patrice Tremblay                                *
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

#include "urlnavigator.h"

#include <assert.h>

#include <qcombobox.h>
#include <qfont.h>
#include <qlabel.h>
#include <q3listbox.h>
#include <qlineedit.h>
#include <qobject.h>
#include <q3popupmenu.h>
#include <qpushbutton.h>
#include <qsizepolicy.h>
#include <qtooltip.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <QKeyEvent>
#include <QCheckBox>

#include <kaction.h>
#include <kactioncollection.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kprotocolinfo.h>
#include <kurl.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kbookmarkmanager.h>

#include "bookmarkselector.h"
#include "dolphin.h"
#include "dolphinsettings.h"
#include "dolphinstatusbar.h"
#include "dolphinview.h"
#include "generalsettings.h"
#include "protocolcombo.h"
#include "urlnavigatorbutton.h"

URLNavigator::HistoryElem::HistoryElem()
 :  m_url(),
    m_currentFileName(),
    m_contentsX(0),
    m_contentsY(0)
{
}

URLNavigator::HistoryElem::HistoryElem(const KUrl& url)
 :  m_url(url),
    m_currentFileName(),
    m_contentsX(0),
    m_contentsY(0)
{
}

URLNavigator::HistoryElem::~HistoryElem()
{
}

URLNavigator::URLNavigator(const KUrl& url,
                           DolphinView* dolphinView) :
    Q3HBox(dolphinView),
    m_historyIndex(0),
    m_dolphinView(dolphinView),
    m_protocols(0),
    m_protocolSeparator(0),
    m_host(0)
{
    m_history.prepend(HistoryElem(url));

    QFontMetrics fontMetrics(font());
    setMinimumHeight(fontMetrics.height() + 8);

    m_toggleButton = new QCheckBox(this);
    //m_toggleButton->setFlat(true);
    //m_toggleButton->setToggleButton(true);
    m_toggleButton->setFocusPolicy(Qt::NoFocus);
    m_toggleButton->setMinimumHeight(minimumHeight());
    connect(m_toggleButton, SIGNAL(clicked()),
            this, SLOT(slotClicked()));
    if (DolphinSettings::instance().generalSettings()->editableURL()) {
        m_toggleButton->toggle();
    }

    m_bookmarkSelector = new BookmarkSelector(this);
    connect(m_bookmarkSelector, SIGNAL(bookmarkActivated(int)),
            this, SLOT(slotBookmarkActivated(int)));

    m_pathBox = new KUrlComboBox(KUrlComboBox::Directories, true, this);

    KUrlCompletion* kurlCompletion = new KUrlCompletion(KUrlCompletion::DirCompletion);
    m_pathBox->setCompletionObject(kurlCompletion);
    m_pathBox->setAutoDeleteCompletionObject(true);

    connect(m_pathBox, SIGNAL(returnPressed(const QString&)),
            this, SLOT(slotReturnPressed(const QString&)));
    connect(m_pathBox, SIGNAL(urlActivated(const KUrl&)),
            this, SLOT(slotURLActivated(const KUrl&)));

    connect(dolphinView, SIGNAL(contentsMoved(int, int)),
            this, SLOT(slotContentsMoved(int, int)));
    connect(dolphinView, SIGNAL(redirection(const KUrl&, const KUrl&)),
            this, SLOT(slotRedirection(const KUrl&, const KUrl&)));
/*    connect(dolphinView, SIGNAL(redirection(const KUrl&)),
            this, SLOT(slotRedirection(const KUrl&)));*/
    updateContent();
}

URLNavigator::~URLNavigator()
{
}

void URLNavigator::setURL(const KUrl& url)
{
    QString urlStr(url.pathOrUrl());
    //kdDebug() << "setURL(" << url << ")" << endl;
    if (urlStr.at(0) == '~') {
        // replace '~' by the home directory
        urlStr.remove(0, 1);
        urlStr.insert(0, QDir::home().path());
    }

    const KUrl transformedURL(urlStr);

    if (m_historyIndex > 0) {
        // Check whether the previous element of the history has the same URL.
        // If yes, just go forward instead of inserting a duplicate history
        // element.
        const KUrl& nextURL = m_history[m_historyIndex - 1].url();
        if (transformedURL == nextURL) {
            goForward();
//             kdDebug() << "goin' forward in history" << endl;
            return;
        }
    }

    const KUrl& currURL = m_history[m_historyIndex].url();
    if (currURL == transformedURL) {
        // don't insert duplicate history elements
//         kdDebug() << "currURL == transformedURL" << endl;
        return;
    }

    updateHistoryElem();

    const Q3ValueListIterator<URLNavigator::HistoryElem> it = m_history.at(m_historyIndex);
    m_history.insert(it, HistoryElem(transformedURL));

    updateContent();

    emit urlChanged(transformedURL);
    emit historyChanged();

    // Prevent an endless growing of the history: remembering
    // the last 100 URLs should be enough...
    if (m_historyIndex > 100) {
        m_history.erase(m_history.begin());
        --m_historyIndex;
    }

/*    kdDebug() << "history starting ====================" << endl;
    int i = 0;
    for (QValueListIterator<URLNavigator::HistoryElem> it = m_history.begin();
         it != m_history.end();
         ++it, ++i)
    {
        kdDebug() << i << ": " << (*it).url() << endl;
    }
    kdDebug() << "history done ========================" << endl;*/
}

const KUrl& URLNavigator::url() const
{
    assert(!m_history.empty());
    return m_history[m_historyIndex].url();
}

KUrl URLNavigator::url(int index) const
{
    assert(index >= 0);
    QString path(url().pathOrUrl());
    path = path.section('/', 0, index);

    if (path.at(path.length()) != '/')
    {
        path.append('/');
    }

    return path;
}

const Q3ValueList<URLNavigator::HistoryElem>& URLNavigator::history(int& index) const
{
    index = m_historyIndex;
    return m_history;
}

void URLNavigator::goBack()
{
    updateHistoryElem();

    const int count = m_history.count();
    if (m_historyIndex < count - 1) {
        ++m_historyIndex;
        updateContent();
        emit urlChanged(url());
        emit historyChanged();
    }
}

void URLNavigator::goForward()
{
    if (m_historyIndex > 0) {
        --m_historyIndex;
        updateContent();
        emit urlChanged(url());
        emit historyChanged();
    }
}

void URLNavigator::goUp()
{
    setURL(url().upUrl());
}

void URLNavigator::goHome()
{
    setURL(DolphinSettings::instance().generalSettings()->homeURL());
}

void URLNavigator::setURLEditable(bool editable)
{
    if (isURLEditable() != editable) {
        m_toggleButton->toggle();
        slotClicked();
    }
}

bool URLNavigator::isURLEditable() const
{
    return m_toggleButton->isChecked();
}

void URLNavigator::editURL(bool editOrBrowse)
{
    setURLEditable(editOrBrowse);

    if (editOrBrowse)
    {
        m_pathBox->setFocus();
    }
}

DolphinView* URLNavigator::dolphinView() const
{
    return m_dolphinView;
}

void URLNavigator::keyReleaseEvent(QKeyEvent* event)
{
    Q3HBox::keyReleaseEvent(event);
    if (isURLEditable() && (event->key() == Qt::Key_Escape)) {
        setURLEditable(false);
    }
}

void URLNavigator::slotReturnPressed(const QString& text)
{
    // Parts of the following code have been taken
    // from the class KateFileSelector located in
    // kate/app/katefileselector.hpp of Kate.
    // Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
    // Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
    // Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

    KUrl typedURL(text);
    if (typedURL.hasPass()) {
        typedURL.setPass(QString::null);
    }

    QStringList urls = m_pathBox->urls();
    urls.remove(typedURL.url());
    urls.prepend(typedURL.url());
    m_pathBox->setUrls(urls, KUrlComboBox::RemoveBottom);

    setURL(typedURL);
    // The URL might have been adjusted by URLNavigator::setURL(), hence
    // synchronize the result in the path box.
    m_pathBox->setUrl(url());
}

void URLNavigator::slotURLActivated(const KUrl& url)
{
    setURL(url);
}

void URLNavigator::slotRemoteHostActivated()
{
    KUrl u = url();

    QString host = m_host->text();
    QString user;

    int marker = host.find("@");
    if (marker != -1)
    {
        user = host.left(marker);
        u.setUser(user);
        host = host.right(host.length() - marker - 1);
    }

    marker = host.find("/");
    if (marker != -1)
    {
        u.setPath(host.right(host.length() - marker));
        host.truncate(marker);
    }
    else
    {
        u.setPath("");
    }

    if (m_protocols->currentProtocol() != u.protocol() ||
        host != u.host() ||
        user != u.user())
    {
        u.setProtocol(m_protocols->currentProtocol());
        u.setHost(m_host->text());

        //TODO: get rid of this HACK for file:///!
        if (u.protocol() == "file")
        {
            u.setHost("");
            if (u.path().isEmpty())
            {
                u.setPath("/");
            }
        }

        setURL(u);
    }
}

void URLNavigator::slotProtocolChanged(const QString& protocol)
{
    KUrl url;
    url.setProtocol(protocol);
    //url.setPath(KProtocolInfo::protocolClass(protocol) == ":local" ? "/" : "");
    url.setPath("/");
    Q3ValueList<QWidget*>::const_iterator it = m_navButtons.constBegin();
    while (it != m_navButtons.constEnd()) {
        (*it)->close();
        (*it)->deleteLater();
        ++it;
    }
    m_navButtons.clear();

    if (KProtocolInfo::protocolClass(protocol) == ":local") {
        setURL(url);
    }
    else {
        if (!m_host) {
            m_protocolSeparator = new QLabel("://", this);
            m_host = new QLineEdit(this);

            connect(m_host, SIGNAL(lostFocus()),
                    this, SLOT(slotRemoteHostActivated()));
            connect(m_host, SIGNAL(returnPressed()),
                    this, SLOT(slotRemoteHostActivated()));
        }
        else {
            m_host->setText("");
        }
        m_protocolSeparator->show();
        m_host->show();
        m_host->setFocus();
    }
}

void URLNavigator::slotRequestActivation()
{
    m_dolphinView->requestActivation();
}

void URLNavigator::slotBookmarkActivated(int index)
{
    m_dolphinView->statusBar()->clear();
    m_dolphinView->requestActivation();

    KBookmark bookmark = DolphinSettings::instance().bookmark(index);
    m_dolphinView->setURL(bookmark.url());
}

void URLNavigator::slotRedirection(const KUrl& oldUrl, const KUrl& newUrl)
{
// kdDebug() << "received redirection to " << newUrl << endl;
kdDebug() << "received redirection from " << oldUrl << " to " << newUrl << endl;
/*    UrlStack::iterator it = m_urls.find(oldUrl);
    if (it != m_urls.end())
    {
        m_urls.erase(++it, m_urls.end());
    }

    m_urls.append(newUrl);*/
}

void URLNavigator::slotContentsMoved(int x, int y)
{
    m_history[m_historyIndex].setContentsX(x);
    m_history[m_historyIndex].setContentsY(y);
}

void URLNavigator::slotClicked()
{
    if (isURLEditable()) {
        m_pathBox->setFocus();
        updateContent();
    }
    else {
        setURL(m_pathBox->currentText());
        m_dolphinView->setFocus();
    }
}

void URLNavigator::updateHistoryElem()
{
    assert(m_historyIndex >= 0);
    const KFileItem* item = m_dolphinView->currentFileItem();
    if (item != 0) {
        m_history[m_historyIndex].setCurrentFileName(item->name());
    }
    m_history[m_historyIndex].setContentsX(m_dolphinView->contentsX());
    m_history[m_historyIndex].setContentsY(m_dolphinView->contentsY());
}

void URLNavigator::updateContent()
{
    // delete all existing URL navigator buttons
    Q3ValueList<QWidget*>::const_iterator it = m_navButtons.constBegin();
    while (it != m_navButtons.constEnd()) {
        (*it)->close();
        (*it)->deleteLater();
        ++it;
    }
    m_navButtons.clear();

    m_bookmarkSelector->updateSelection(url());

    QToolTip::remove(m_toggleButton);
    QString path(url().pathOrUrl());
    const KAction* action = Dolphin::mainWin().actionCollection()->action("editable_location");
    // TODO: registry of default shortcuts
    QString shortcut = action? action->shortcutText() : "Ctrl+L";
    if (m_toggleButton->isChecked()) {
        delete m_protocols; m_protocols = 0;
        delete m_protocolSeparator; m_protocolSeparator = 0;
        delete m_host; m_host = 0;

        QToolTip::add(m_toggleButton, i18n("Browse (%1, Escape)").arg(shortcut));

        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_pathBox->show();
        m_pathBox->setUrl(url());
    }
    else {
        QToolTip::add(m_toggleButton, i18n("Edit location (%1)").arg(shortcut));

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_pathBox->hide();
        QString dir_name;

        // get the data from the currently selected bookmark
        KBookmark bookmark = m_bookmarkSelector->selectedBookmark();
        //int bookmarkIndex = m_bookmarkSelector->selectedIndex();

        QString bookmarkPath;
        if (bookmark.isNull()) {
            // No bookmark is a part of the current URL.
            // The following code tries to guess the bookmark
            // path. E. g. "fish://root@192.168.0.2/var/lib" writes
            // "fish://root@192.168.0.2" to 'bookmarkPath', which leads to the
            // navigation indication 'Custom Path > var > lib".
            int idx = path.find(QString("//"));
            idx = path.find("/", (idx < 0) ? 0 : idx + 2);
            bookmarkPath = (idx < 0) ? path : path.left(idx);
        }
        else {
            bookmarkPath = bookmark.url().pathOrUrl();
        }
        const uint len = bookmarkPath.length();

        // calculate the start point for the URL navigator buttons by counting
        // the slashs inside the bookmark URL
        int slashCount = 0;
        for (uint i = 0; i < len; ++i) {
            if (bookmarkPath.at(i) == QChar('/')) {
                ++slashCount;
            }
        }
        if ((len > 0) && bookmarkPath.at(len - 1) == QChar('/')) {
            assert(slashCount > 0);
            --slashCount;
        }

        if (!url().isLocalFile() && bookmark.isNull()) {
            QString protocol = url().protocol();
            if (!m_protocols) {
                m_protocols = new ProtocolCombo(protocol, this);
                connect(m_protocols, SIGNAL(activated(const QString&)),
                        this, SLOT(slotProtocolChanged(const QString&)));
            }
            else {
                m_protocols->setProtocol(protocol);
            }
            m_protocols->show();

            if (KProtocolInfo::protocolClass(protocol) != ":local")
            {
                QString hostText = url().host();

                if (!url().user().isEmpty())
                {
                    hostText = url().user() + "@" + hostText;
                }

                if (!m_host) {
                    m_protocolSeparator = new QLabel("://", this);
                    m_host = new QLineEdit(hostText, this);

                    connect(m_host, SIGNAL(lostFocus()),
                            this, SLOT(slotRemoteHostActivated()));
                    connect(m_host, SIGNAL(returnPressed()),
                            this, SLOT(slotRemoteHostActivated()));
                }
                else {
                    m_host->setText(hostText);
                }
                m_protocolSeparator->show();
                m_host->show();
            }
            else {
                delete m_protocolSeparator; m_protocolSeparator = 0;
                delete m_host; m_host = 0;
            }
        }
        else if (m_protocols) {
            m_protocols->hide();

            if (m_host) {
                m_protocolSeparator->hide();
                m_host->hide();
            }
        }

        // create URL navigator buttons
        int idx = slashCount;
        bool hasNext = true;
        do {
            dir_name = path.section('/', idx, idx);
            const bool isFirstButton = (idx == slashCount);
            hasNext = isFirstButton || !dir_name.isEmpty();
            if (hasNext) {
                URLNavigatorButton* button = new URLNavigatorButton(idx, this);
                if (isFirstButton) {
                    // the first URL navigator button should get the name of the
                    // bookmark instead of the directory name
                    QString text = bookmark.text();
                    if (text.isEmpty()) {
                        if (url().isLocalFile())
                        {
                            text = i18n("Custom Path");
                        }
                        else
                        {
                            delete button;
                            ++idx;
                            continue;
                        }
                    }
                    button->setText(text);
                }
                button->show();
                m_navButtons.append(button);
                ++idx;
            }
        } while (hasNext);
    }
}

#include "urlnavigator.moc"
