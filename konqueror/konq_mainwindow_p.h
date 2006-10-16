/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_mainwindow_p_h__
#define __konq_mainwindow_p_h__

class KonqExtendedBookmarkOwner : public QObject, public KonqBookmarkOwner
{
public:
  KonqExtendedBookmarkOwner(KonqMainWindow *);
  virtual QString currentTitle() const;
  virtual QString currentUrl() const;
  virtual bool supportsTabs() const;
  virtual QList<QPair<QString, QString> > currentBookmarkList() const;
  virtual void openBookmark(const KBookmark & bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km);
  virtual void openInNewTab(KBookmark bm);
  virtual void openInNewWindow(KBookmark bm);
  virtual void openFolderinTabs(KBookmark bm);

private:
  KonqMainWindow *m_pKonqMainWindow;
};

#endif
