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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qregexp.h>
#include <kdebug.h>
#include <klocale.h>

#include <kmessagebox.h>
#include <kfiledialog.h>

#include <kbookmarkmanager.h>

#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>

#include "commands.h"
#include "toplevel.h"
#include "listview.h"
#include "mymanager.h"

#include "importers.h"

int ImportCommand::doImport(QWidget *parent, QString imp) {
   int answer = KMessageBox::questionYesNoCancel(
                   parent, i18n("Import as a new subfolder or replace all the current bookmarks?"),
                   i18n("%1 Import").arg(imp), i18n("As New Folder"), i18n("Replace"));
   return (answer == KMessageBox::Cancel) ? 0 : ((answer == KMessageBox::Yes) ? 2 : 1);
}

QString ImportCommand::name() const {
   return i18n("Import %1 Bookmarks").arg(visibleName());
}

QString ImportCommand::folder()const {
   return m_folder ? (i18n("%1 Bookmarks").arg(visibleName())) : (QString::null);
}

void ImportCommand::execute() {
   KBookmarkGroup bkGroup;

   if (folder().isEmpty()) {
      // import into the root, after cleaning it up
      bkGroup = BkManagerAccessor::mgr()->root();
      delete m_cleanUpCmd;
      m_cleanUpCmd = DeleteCommand::deleteAll( bkGroup );

      // unselect current item, it doesn't exist anymore
      listview->clearSelection();
      m_cleanUpCmd->execute();

      // import at the root
      m_group = "";

   } else {
      doCreateHoldingFolder(bkGroup);
   }

   doExecuteWrapper(bkGroup);
}

void ImportCommand::unexecute() {
   if ( !folder().isEmpty() ) {
      // we created a group -> just delete it
      DeleteCommand cmd(m_group);
      cmd.execute();

   } else {
      // we imported at the root -> delete everything
      KBookmarkGroup root = BkManagerAccessor::mgr()->root();
      KCommand *cmd = DeleteCommand::deleteAll(root);

      // unselect current item, it doesn't exist anymore
      listview->clearSelection();
      cmd->execute();
      delete cmd;

      // and recreate what was there before
      m_cleanUpCmd->unexecute();
   }
}

void ImportCommand::newBookmark(const QString &text, const QCString &url, const QString &additionnalInfo) {
   KBookmark bk = m_stack.top()->addBookmark(
                                    BkManagerAccessor::mgr(),
                                    text, QString::fromUtf8(url),
                                    QString::null, false);
   // store additionnal info
   bk.internalElement().setAttribute("netscapeinfo", additionnalInfo);
}

void ImportCommand::newFolder( const QString & text, bool open, const QString & additionnalInfo ) {
   // we use a qvaluelist so that we keep pointers to valid objects in the stack
   m_list.append(m_stack.top()->createNewFolder(BkManagerAccessor::mgr(), text, false));
   m_stack.push(&(m_list.last()));

   // store additionnal info
   QDomElement element = m_list.last().internalElement();
   element.setAttribute("netscapeinfo", additionnalInfo);
   element.setAttribute("folded", (open?"no":"yes"));
}

void ImportCommand::newSeparator() {
   m_stack.top()->createNewSeparator();
}

void ImportCommand::endFolder() {
   m_stack.pop();
}

void ImportCommand::doCreateHoldingFolder(KBookmarkGroup &bkGroup) {
   // TODO - why the hell isn't this called for XBEL???
   bkGroup = BkManagerAccessor::mgr()
      ->root().createNewFolder(BkManagerAccessor::mgr(), folder(), false);
   bkGroup.internalElement().setAttribute("icon", m_icon);
   m_group = bkGroup.address();
}

void ImportCommand::doExecuteWrapper(const KBookmarkGroup bkGroup) {
   // setup some things we need before executing
   m_stack.push(&bkGroup);
   doExecute();
   m_list.clear();
   m_stack.clear();
}

void ImportCommand::connectImporter(const QObject *importer) {
   connect(importer, SIGNAL( newBookmark(const QString &, const QCString &, const QString &) ),
                     SLOT( newBookmark(const QString &, const QCString &, const QString &) ));
   connect(importer, SIGNAL( newFolder(const QString &, bool, const QString &) ),
                     SLOT( newFolder(const QString &, bool, const QString &) ));
   connect(importer, SIGNAL( newSeparator() ),
                     SLOT( newSeparator() ) );
   connect(importer, SIGNAL( endFolder() ),
                     SLOT( endFolder() ) );
}

// importer subclasses

OperaImportCommand::OperaImportCommand(const QString &fileName, bool folder)
   : ImportCommand(fileName, folder, "opera", false) {
   ;
}

QString OperaImportCommand::requestFilename() const {
   return KOperaBookmarkImporter::operaBookmarksFile();
}

QString OperaImportCommand::visibleName() const {
   return i18n("Opera");
}

void OperaImportCommand::doExecute() {
   KOperaBookmarkImporter importer(m_fileName);
   connectImporter(&importer);
   importer.parseOperaBookmarks();
}

IEImportCommand::IEImportCommand(const QString &fileName, bool folder)
   : ImportCommand(fileName, folder, "", false) {
   ;
}

QString IEImportCommand::requestFilename() const {
   return KIEBookmarkImporter::IEBookmarksDir();
}

QString IEImportCommand::visibleName() const {
   return i18n("IE");
}

void IEImportCommand::doExecute() {
   KIEBookmarkImporter importer(m_fileName);
   connectImporter(&importer);
   importer.parseIEBookmarks();
}

void HTMLImportCommand::doExecute() {
   KNSBookmarkImporter importer(m_fileName);
   connectImporter(&importer);
   importer.parseNSBookmarks(m_utf8);
}

MozImportCommand::MozImportCommand(const QString &fileName, bool folder)
   : HTMLImportCommand(fileName, folder, "mozilla", true) {
   ;
}

QString MozImportCommand::requestFilename() const {
   return KNSBookmarkImporter::mozillaBookmarksFile();
}

QString MozImportCommand::visibleName() const {
   return i18n("Mozilla");
}

NSImportCommand::NSImportCommand(const QString &fileName, bool folder)
   : HTMLImportCommand(fileName, folder, "netscape", false) {
   ;
}

QString NSImportCommand::requestFilename() const {
   return KNSBookmarkImporter::netscapeBookmarksFile();
}

QString NSImportCommand::visibleName() const {
   return i18n("Netscape");
}

HTMLImportCommand::HTMLImportCommand(const QString &fileName, bool folder,
      const QString &icon, bool utf8)
   : ImportCommand(fileName, folder, icon, utf8) {
   ;
}

XBELImportCommand::XBELImportCommand(const QString &fileName, bool folder, const QString &icon)
   : ImportCommand(fileName, folder, icon, false) {
   ;
}

GaleonImportCommand::GaleonImportCommand(const QString &fileName, bool folder)
   : XBELImportCommand(fileName, folder, "") {
   ;
}

QString GaleonImportCommand::requestFilename() const {
   return KFileDialog::getOpenFileName(
               QDir::homeDirPath() + "/.galeon",
               i18n("*.xbel|Galeon bookmark files (*.xbel)"));
}

QString GaleonImportCommand::visibleName() const {
   return i18n("Galeon");
}

KDE2ImportCommand::KDE2ImportCommand(const QString &fileName, bool folder)
   : XBELImportCommand(fileName, folder, "") {
   ;
}

QString KDE2ImportCommand::requestFilename() const {
   // locateLocal on the bookmarks file and get dir?
   return KFileDialog::getOpenFileName(
               QDir::homeDirPath() + "/.kde",
               i18n("*.xml|KDE bookmark files (*.xml)"));
}

QString KDE2ImportCommand::visibleName() const {
   return i18n("KDE");
}

void XBELImportCommand::doCreateHoldingFolder(KBookmarkGroup &) {
   ;
}

void XBELImportCommand::doExecuteWrapper(const KBookmarkGroup) {
   doExecute();
}

void XBELImportCommand::doExecute() {
   // TODO - there is probably a bug somewhere in here
   //        that adds unneeded data to the dom, FIXME!
   KBookmarkManager *pManager;
   pManager = KBookmarkManager::managerForFile(m_fileName, false);
   QDomDocument doc = BkManagerAccessor::mgr()->internalDocument();

   // get the xbel
   QDomNode subDoc = pManager->internalDocument().namedItem("xbel").cloneNode();

   if (!folder().isEmpty()) {
      // transform into folder
      subDoc.toElement().setTagName("folder");

      // clear attributes
      QStringList tags;
      for (uint i = 0; i < subDoc.attributes().count(); i++) {
         tags << subDoc.attributes().item(i).toAttr().name();
      }

      for (QStringList::Iterator it = tags.begin(); it != tags.end(); ++it) {
         subDoc.attributes().removeNamedItem((*it));
      }

      subDoc.toElement().setAttribute("icon", m_icon);

      // give the folder a name
      QDomElement textElem = doc.createElement("title");
      subDoc.insertBefore(textElem, subDoc.firstChild());
      textElem.appendChild(doc.createTextNode(folder()));
   }

   // import and add it
   QDomNode node = doc.importNode(subDoc, true);

   if (!folder().isEmpty()) {
      BkManagerAccessor::mgr()->root().internalElement().appendChild(node);
      m_group = KBookmarkGroup(node.toElement()).address();

   } else {
      QDomElement root = BkManagerAccessor::mgr()->root().internalElement();

      QValueList<QDomElement> childList;

      QDomNode n = subDoc.firstChild().toElement();

      while (!n.isNull()) {
         QDomElement e = n.toElement();
         if (!e.isNull()) {
            childList.append(e);
         }
         n = n.nextSibling();
      }

      QValueList<QDomElement>::Iterator it = childList.begin();
      QValueList<QDomElement>::Iterator end = childList.end();
      for (; it!= end ; ++it) {
         root.appendChild((*it));
      }
   }
}

template <class TheImporter>
static TheImporter* callImporter(QWidget *parent)
{
   TheImporter* importer = new TheImporter();
   QString mydirname = importer->requestFilename();
   if (mydirname.isEmpty()) {
      return 0;
   }
   int ret = TheImporter::doImport(parent, importer->visibleName());
   return (ret == 0) ? 0 : new TheImporter(mydirname, (ret == 2));
}

ImportCommand* ImportCommandFactory::call(const QCString &type, QWidget *top) {
   if (type == "Galeon") return callImporter<GaleonImportCommand>(top);
   if (type == "IE")     return callImporter<IEImportCommand>    (top);
   if (type == "KDE2")   return callImporter<KDE2ImportCommand>  (top);
   if (type == "Opera")  return callImporter<OperaImportCommand> (top);
   if (type == "Moz")    return callImporter<MozImportCommand>   (top);
   if (type == "NS")     return callImporter<NSImportCommand>    (top);
}

#include "importers.moc"
