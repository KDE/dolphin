/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __kfm_run_h__
#define __kfm_run_h__

#include <krun.h>
#include <qguardedptr.h>
#include <kservice.h>
#include <sys/types.h>
#include <konq_openurlrequest.h>

class KonqMainWindow;
class KonqView;

class KonqRun : public KRun
{
  Q_OBJECT
public:
  /**
   * Create a KonqRun instance, associated to the main view and an
   * optionnal child view.
   */
  KonqRun( KonqMainWindow* mainWindow, KonqView *childView,
                 const KURL &url, const KonqOpenURLRequest & req = KonqOpenURLRequest(),
           bool trustedSource = false );

  virtual ~KonqRun();

  /**
   * Returns true if we found the servicetype for the given url.
   */
  bool foundMimeType() const { return m_bFoundMimeType; }

  KonqView *childView() const { return m_pView; }

  const QString & typedURL() const { return m_req.typedURL; }
  KURL url() const { return m_strURL; }

  static bool allowExecution( const QString &serviceType, const KURL &url );
  static bool isExecutable( const QString &serviceType );
  static bool askSave( const KURL & url, KService::Ptr offer, const QString& mimeType, const QString & suggestedFilename = QString::null );
  static void save( const KURL & url, const QString & suggestedFilename );

protected:
  /**
   * Called if the mimetype has been detected. The function checks wether the document
   * and appends the gzip protocol to the URL. Otherwise @ref #runURL is called to
   * finish the job.
   */
  virtual void foundMimeType( const QString & _type );

  virtual void scanFile();

protected slots:
  void slotKonqScanFinished(KIO::Job *job);
  void slotKonqMimetype(KIO::Job *job, const QString &type);

protected:
  QGuardedPtr<KonqMainWindow> m_pMainWindow;
  QGuardedPtr<KonqView> m_pView;
  bool m_bFoundMimeType;
  KonqOpenURLRequest m_req;
  bool m_bTrustedSource;
  QString m_suggestedFilename;
};

#endif
