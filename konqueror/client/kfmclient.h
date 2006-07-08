/* This file is part of the KDE project
   Copyright (C) 1999-2006 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __kfmclient_h
#define __kfmclient_h

#include <QApplication>
class KJob;
class KUrl;

class ClientApp : public QApplication
{
  Q_OBJECT
public:
  ClientApp(int &argc, char **argv ) : QApplication( argc, argv ) {}

  /** Parse command-line arguments and "do it" */
  static bool doIt();

  /** Make konqueror open a window for @p url */
  static bool createNewWindow(const KUrl & url, bool newTab, bool tempFile, const QString & mimetype = QString());

  /** Make konqueror open a window for @p profile, @p url and @p mimetype */
  static bool openProfile(const QString & profile, const QString & url, const QString & mimetype = QString());

protected Q_SLOTS:
  void slotResult( KJob * );
  void delayedQuit();
  void slotDialogCanceled();

private:
  static void sendASNChange();
  static bool m_ok;
  static QByteArray startup_id_str;

};

#endif
