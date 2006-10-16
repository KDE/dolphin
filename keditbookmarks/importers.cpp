// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "importers.h"

#include "commands.h"
#include "toplevel.h"

#include <QRegExp>
#include <kdebug.h>
#include <klocale.h>

#include <kmessagebox.h>
#include <kfiledialog.h>

#include <kbookmark.h>
#include <kbookmarkmanager.h>

#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
#include <kbookmarkimporter_crash.h>
#include <kbookmarkdombuilder.h>

QString ImportCommand::name() const {
    return i18n("Import %1 Bookmarks", visibleName());
}

QString ImportCommand::folder() const {
    return m_folder ? i18n("%1 Bookmarks", visibleName()) : QString();
}

ImportCommand* ImportCommand::importerFactory(const QString &type) {
    if (type == "Galeon") return new GaleonImportCommand();
    else if (type == "IE") return new IEImportCommand();
    else if (type == "KDE2") return new KDE2ImportCommand();
    else if (type == "Opera") return new OperaImportCommand();
    else if (type == "Crashes") return new CrashesImportCommand();
    else if (type == "Moz") return new MozImportCommand();
    else if (type == "NS") return new NSImportCommand();
    else {
        kError() << "ImportCommand::importerFactory() - invalid type (" << type << ")!" << endl;
        return 0;
    }
}

ImportCommand* ImportCommand::performImport(const QString &type, QWidget *top) {
    ImportCommand *importer = ImportCommand::importerFactory(type);

    QString mydirname = importer->requestFilename();
    if (mydirname.isEmpty()) {
        delete importer;
        return 0;
    }

    int answer =
        KMessageBox::questionYesNoCancel(
                top, i18n("Import as a new subfolder or replace all the current bookmarks?"),
                i18n("%1 Import", importer->visibleName()),
                KGuiItem(i18n("As New Folder")), KGuiItem(i18n("Replace")));

    if (answer == KMessageBox::Cancel) {
        delete importer;
        return 0;
    }

    importer->import(mydirname, answer == KMessageBox::Yes);
    return importer;
}

void ImportCommand::doCreateHoldingFolder(KBookmarkGroup &bkGroup) {
    bkGroup = CurrentMgr::self()->mgr()
        ->root().createNewFolder(CurrentMgr::self()->mgr(), folder(), false);
    bkGroup.internalElement().setAttribute("icon", m_icon);
    m_group = bkGroup.address();
}

void ImportCommand::execute() {
    KBookmarkGroup bkGroup;

    if (!folder().isNull()) {
        doCreateHoldingFolder(bkGroup);

    } else {
        // import into the root, after cleaning it up
        bkGroup = CurrentMgr::self()->root();
        delete m_cleanUpCmd;
        m_cleanUpCmd = DeleteCommand::deleteAll(bkGroup);

        KMacroCommand *mcmd = (KMacroCommand*) m_cleanUpCmd;
        mcmd->addCommand(new DeleteCommand(bkGroup.address(),
                    true /* contentOnly */));
        m_cleanUpCmd->execute();

        // import at the root
        m_group = "";
    }

    doExecute(bkGroup);
}

void ImportCommand::unexecute() {
    if ( !folder().isEmpty() ) {
        // we created a group -> just delete it
        DeleteCommand cmd(m_group);
        cmd.execute();

    } else {
        // we imported at the root -> delete everything
        KBookmarkGroup root = CurrentMgr::self()->root();
        KCommand *cmd = DeleteCommand::deleteAll(root);

        cmd->execute();
        delete cmd;

        // and recreate what was there before
        m_cleanUpCmd->unexecute();
    }
}

QString ImportCommand::affectedBookmarks() const
{
    QString rootAdr = CurrentMgr::self()->root().address();
    if(m_group == rootAdr)
        return m_group;
    else
        return KBookmark::parentAddress(m_group);
}

/* -------------------------------------- */

QString MozImportCommand::requestFilename() const {
    static KMozillaBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

QString NSImportCommand::requestFilename() const {
    static KNSBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

QString OperaImportCommand::requestFilename() const {
    static KOperaBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

QString CrashesImportCommand::requestFilename() const {
    static KCrashBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

QString IEImportCommand::requestFilename() const {
    static KIEBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

// following two are really just xbel

QString GaleonImportCommand::requestFilename() const {
    return KFileDialog::getOpenFileName(
            QDir::homePath() + "/.galeon",
            i18n("*.xbel|Galeon Bookmark Files (*.xbel)"));
}

#include "kstandarddirs.h"

QString KDE2ImportCommand::requestFilename() const {
    return KFileDialog::getOpenFileName(
            KStandardDirs::locateLocal("data", "konqueror"),
            i18n("*.xml|KDE Bookmark Files (*.xml)"));
}

/* -------------------------------------- */

static void parseInto(const KBookmarkGroup &bkGroup, KBookmarkImporterBase *importer) {
    KBookmarkDomBuilder builder(bkGroup, CurrentMgr::self()->mgr());
    builder.connectImporter(importer);
    importer->parse();
}

void OperaImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
    KOperaBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    parseInto(bkGroup, &importer);
}

void CrashesImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
    KCrashBookmarkImporterImpl importer;
    importer.setShouldDelete(true);
    importer.setFilename(m_fileName);
    parseInto(bkGroup, &importer);
}

void IEImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
    KIEBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    parseInto(bkGroup, &importer);
}

void HTMLImportCommand::doExecute(const KBookmarkGroup &bkGroup) {
    KNSBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    importer.setUtf8(m_utf8);
    parseInto(bkGroup, &importer);
}

/* -------------------------------------- */

void XBELImportCommand::doCreateHoldingFolder(KBookmarkGroup &) {
    // rather than reuse the old group node we transform the
    // root xbel node into the group when doing an xbel import
}

void XBELImportCommand::doExecute(const KBookmarkGroup &/*bkGroup*/) {
    // check if already open first???
    KBookmarkManager *pManager = KBookmarkManager::managerForFile(m_fileName,"", false);

    QDomDocument doc = CurrentMgr::self()->mgr()->internalDocument();

    // get the xbel
    QDomNode subDoc = pManager->internalDocument().namedItem("xbel").cloneNode();
    if (subDoc.isProcessingInstruction())
        subDoc = subDoc.nextSibling();
    if (subDoc.isDocumentType())
        subDoc = subDoc.nextSibling();
    if (subDoc.nodeName() != "xbel")
        return;

    if (!folder().isEmpty()) {
        // transform into folder
        subDoc.toElement().setTagName("folder");

        // clear attributes
        QStringList tags;
        for (int i = 0; i < subDoc.attributes().count(); i++)
            tags << subDoc.attributes().item(i).toAttr().name();
        for (QStringList::Iterator it = tags.begin(); it != tags.end(); ++it)
            subDoc.attributes().removeNamedItem((*it));

        subDoc.toElement().setAttribute("icon", m_icon);

        // give the folder a name
        QDomElement textElem = doc.createElement("title");
        subDoc.insertBefore(textElem, subDoc.firstChild());
        textElem.appendChild(doc.createTextNode(folder()));
    }

    // import and add it
    QDomNode node = doc.importNode(subDoc, true);

    if (!folder().isEmpty()) {
        CurrentMgr::self()->root().internalElement().appendChild(node);
        m_group = KBookmarkGroup(node.toElement()).address();

    } else {
        QDomElement root = CurrentMgr::self()->root().internalElement();

        QList<QDomElement> childList;

        QDomNode n = subDoc.firstChild().toElement();
        while (!n.isNull()) {
            QDomElement e = n.toElement();
            if (!e.isNull())
                childList.append(e);
            n = n.nextSibling();
        }

        QList<QDomElement>::Iterator it = childList.begin();
        QList<QDomElement>::Iterator end = childList.end();
        for (; it!= end ; ++it)
            root.appendChild((*it));
    }
}

#include "importers.moc"
