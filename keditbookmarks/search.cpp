// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "toplevel.h"
#include "listview.h"
#include "search.h"
#include "commands.h"

#include <qregexp.h>
#include <qtimer.h>

#include <kdebug.h>
#include <klocale.h>
#include <kfind.h>

#include <kbookmarkmanager.h>

MagicKLineEdit::MagicKLineEdit(
   const QString &text, QWidget *parent, const char *name
) : KLineEdit(text, parent, name), m_grayedText(text) {
    setPaletteForegroundColor(gray);
}

void MagicKLineEdit::focusInEvent(QFocusEvent *ev) {
    if (text() == m_grayedText)
        setText(QString::null);
    QLineEdit::focusInEvent(ev);
}

void MagicKLineEdit::focusOutEvent(QFocusEvent *ev) {
    if (text().isEmpty()) {
        setText(m_grayedText);
        setPaletteForegroundColor(gray); 
    }
    QLineEdit::focusOutEvent(ev);
}

void MagicKLineEdit::mousePressEvent(QMouseEvent *ev) {
    setPaletteForegroundColor(parentWidget()->paletteForegroundColor()); 
    QLineEdit::mousePressEvent(ev);
}

class KBookmarkTextMap : private KBookmarkGroupTraverser {
public:
    KBookmarkTextMap(KBookmarkManager *);
    void update();
    QValueList<KBookmark> find(const QString &text) const;
private:
    virtual void visit(const KBookmark &);
    virtual void visitEnter(const KBookmarkGroup &);
    virtual void visitLeave(const KBookmarkGroup &) { ; }
private:
    typedef QValueList<KBookmark> KBookmarkList;
    QMap<QString, KBookmarkList> m_bk_map;
    KBookmarkManager *m_manager;
};

KBookmarkTextMap::KBookmarkTextMap( KBookmarkManager *manager ) {
    m_manager = manager;
}

void KBookmarkTextMap::update()
{
    m_bk_map.clear();
    KBookmarkGroup root = m_manager->root();
    traverse(root);
}

void KBookmarkTextMap::visit(const KBookmark &bk) {
    if (!bk.isSeparator()) {
        QString text = bk.url().url() + " " + bk.text() + NodeEditCommand::getNodeText(bk, "desc");
        m_bk_map[text].append(bk);
    }
}

void KBookmarkTextMap::visitEnter(const KBookmarkGroup &grp) {
    visit(grp);
}

QValueList<KBookmark> KBookmarkTextMap::find(const QString &text) const
{
    QValueList<KBookmark> matches;
    QValueList<QString> keys = m_bk_map.keys();
    for (QValueList<QString>::iterator it = keys.begin();
            it != keys.end(); ++it 
        ) {
        if ((*it).find(text,0,false) != -1)
            matches += m_bk_map[(*it)];
    }
    return matches;
}

Searcher* Searcher::s_self = 0;

static unsigned int m_currentResult;

class Address
{
public:
    Address() { ; }
    Address(const QString &str) { m_string = str; }
    virtual ~Address() { ; }
    QString string() const { return m_string; }
    bool operator< ( const Address & s2 ) const {
        bool ret = Address::addressStringCompare(string(), s2.string());
        // kdDebug() << string() << " < " << s2.string() << " == " << ret << endl;
        return ret;
    }
    static bool addressStringCompare(const QString & s1, const QString & s2) {
        QStringList s1s = QStringList::split("/", s1);
        QStringList s2s = QStringList::split("/", s2);
        for (unsigned int n = 0; ; n++) {
            if (n >= s1s.count())  // "/0/*5" < "/0"
                return true;
            if (n >= s2s.count())  // "/0" > "/0/*5"
                return false;
            int i1 = s1s[n].toInt();
            int i2 = s2s[n].toInt();
            if (i1 != i2)          // "/*0/2" == "/*0/3"
                return (i1 < i2);   // "/*2" < "/*3"
        }
        // fall through, probably never hit...
        return false;
    }
private:
    QString m_string;
};

static QValueList<Address> m_foundAddrs;

void Searcher::slotSearchNext()
{
    if (m_foundAddrs.empty())
        return;
    QString addr = m_foundAddrs[m_currentResult].string();
    KEBListViewItem *item = ListView::self()->getItemAtAddress(addr);
    m_currentResult = m_currentResult+1 > m_foundAddrs.count()-1
        ? 0 : m_currentResult+1;
    ListView::self()->clearSelection();
    ListView::self()->setCurrent(item);
    item->setSelected(true);
    ListView::self()->emitSlotSelectionChanged();
}

void Searcher::slotSearchTextChanged(const QString & text)
{
    if (text.isEmpty() || text == i18n("Click here and type to search..."))
        return;
    if (!m_bktextmap)
        m_bktextmap = new KBookmarkTextMap(CurrentMgr::self()->mgr());
    m_bktextmap->update(); // FIXME - make this public and use it!!!
    QValueList<KBookmark> results = m_bktextmap->find(text);
    m_foundAddrs.clear();
    for (QValueList<KBookmark>::iterator it = results.begin(); it != results.end(); ++it )
        m_foundAddrs << Address((*it).address());
    // sort the addr list so we don't go "next" in a random order
    qHeapSort(m_foundAddrs.begin(), m_foundAddrs.end());
    m_currentResult = 0;
    slotSearchNext();
}

#include "search.moc"
