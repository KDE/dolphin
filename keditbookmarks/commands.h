// -*- mode:cperl; cperl-indent-level:4; cperl-continued-statement-offset:4; indent-tabs-mode:nil -*-
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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __commands_h
#define __commands_h

#include <kcommand.h>
#include <kbookmark.h>

class CreateCommand : public KCommand
{
public:
   // separator
   CreateCommand(const QString &address, bool konqi = false)
      : KCommand(), m_to(address),
        m_group(false), m_separator(true), m_konqi(konqi), m_originalBookmark(QDomElement())
   { ; }

   // bookmark
   CreateCommand(const QString &address,
                 const QString &text, const QString &iconPath, 
                 const KURL &url, bool konqi = false)
      : KCommand(), m_to(address), m_text(text), m_iconPath(iconPath), m_url(url),
        m_group(false), m_separator(false), m_konqi(konqi), m_originalBookmark(QDomElement())
   { ; }

   // folder
   CreateCommand(const QString &address,
                 const QString &text, const QString &iconPath, 
                 bool open, bool konqi = false)
      : KCommand(), m_to(address), m_text(text), m_iconPath(iconPath),
        m_group(true), m_separator(false), m_open(open), m_konqi(konqi), m_originalBookmark(QDomElement())
   { ; }

   // clone existing bookmark
   CreateCommand(const QString &address,
                 const KBookmark &original, const QString &name = QString::null)
      : KCommand(), m_to(address), m_group(false), m_separator(false),
        m_open(false), m_konqi(false), m_originalBookmark(original), m_mytext(name)
   { ; }

   QString finalAddress() const;

   virtual ~CreateCommand() { ; }
   virtual void execute();
   virtual void unexecute();
   virtual QString name() const;
private:
   QString m_to;
   QString m_text;
   QString m_iconPath;
   KURL m_url;
   bool m_group:1;
   bool m_separator:1;
   bool m_open:1;
   bool m_konqi:1;
   KBookmark m_originalBookmark;
   QString m_mytext;
};

class EditCommand : public KCommand
{
public:

   struct Edition {
      Edition() { ; } // needed for QValueList
      Edition(const QString &a, const QString &v) : attr(a), value(v) {}
      QString attr;
      QString value;
   };

   // change one attribute
   EditCommand(const QString &address, Edition edition, const QString &name = QString::null) 
      : KCommand(), m_address(address), m_mytext(name)
   {
      m_editions.append(edition);
   }

   // change multiple attributes
   EditCommand(const QString &address,
               const QValueList<Edition> &editions, 
               const QString &name = QString::null)
      : KCommand(), m_address(address), m_editions(editions), m_mytext(name)
   { ; }
   virtual ~EditCommand() { ; }
   virtual void execute();
   virtual void unexecute();
   virtual QString name() const;
private:
   QString m_address;
   QValueList<Edition> m_editions;
   QValueList<Edition> m_reverseEditions;
   QString m_mytext;
};

class NodeEditCommand : public KCommand
{
public:
   NodeEditCommand(const QString &address, 
                   const QString &newText, 
                   const QString &nodeName)
      : KCommand(), m_address(address), m_newText(newText), m_nodename(nodeName)
   { ; }
   virtual ~NodeEditCommand() { ; }
   virtual void execute();
   virtual void unexecute();
   virtual QString name() const;
   static QString getNodeText(KBookmark bk, const QStringList &nodehier);
   static QString setNodeText(KBookmark bk, const QStringList &nodehier, 
                              QString newValue);
private:
   QString m_address;
   QString m_newText;
   QString m_oldText;
   QString m_nodename;
};

class DeleteCommand : public KCommand
{
public:
   DeleteCommand(const QString &from, bool contentOnly = false)
      : KCommand(), m_from(from), m_cmd(0L), m_subCmd(0L), m_contentOnly(contentOnly)
   { ; }
   virtual ~DeleteCommand() { delete m_cmd; delete m_subCmd;}
   virtual void execute();
   virtual void unexecute();
   virtual QString name() const { 
      // NOTE - DeleteCommand needs no name, its always embedded in a macrocommand
      return ""; 
   };
   static KMacroCommand* deleteAll(const KBookmarkGroup &parentGroup);
private:
   QString m_from;
   KCommand *m_cmd;
   KMacroCommand *m_subCmd;
   bool m_contentOnly;
};

class MoveCommand : public KCommand
{
public:
   // "Create it with itemsAlreadyMoved=true since 
   // "KListView moves the item before telling us about it."
   MoveCommand(const QString &from, const QString &to, const QString &name = QString::null)
      : KCommand(), m_from(from), m_to(to), m_mytext(name)
   { ; }
   QString finalAddress() const;
   virtual ~MoveCommand() { ; }
   virtual void execute();
   virtual void unexecute();
   virtual QString name() const;
private:
   QString m_from;
   QString m_to;
   QString m_mytext;
};

class SortItem;

class SortCommand : public KMacroCommand
{
public:
   SortCommand(const QString &name, const QString &groupAddress)
      : KMacroCommand(name), m_groupAddress(groupAddress) 
   { ; }
   virtual ~SortCommand() 
   { ; }
   virtual void execute();
   virtual void unexecute();
   // internal
   void moveAfter(const SortItem &moveMe, const SortItem &afterMe);
private:
   QString m_groupAddress;
};

class KEBListViewItem;

class CmdGen {
public:
   static CmdGen* self() { if (!s_self) s_self = new CmdGen(); return s_self; }
   KMacroCommand* setAsToolbar(const KBookmark &bk);
   KMacroCommand* setShownInToolbar(const KBookmark &bk, bool show);
   bool shownInToolbar(const KBookmark &bk);
   KMacroCommand* deleteItems(const QString &commandName, QPtrList<KEBListViewItem> *items);
   KMacroCommand* insertMimeSource(const QString &cmdName, QMimeSource *data, const QString &addr);
   KMacroCommand* itemsMoved(QPtrList<KEBListViewItem> *items, const QString &newAddress, bool copy);
private:
   CmdGen() { ; }
   static CmdGen *s_self;
};

#endif
