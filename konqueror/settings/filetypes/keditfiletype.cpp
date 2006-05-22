/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

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
#include "filetypedetails.h"
#include "typeslistitem.h"
#include "keditfiletype.h"

#include <QFile>

#include <dcopclient.h>
#include <kapplication.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <kcmdlineargs.h>
#include <ksycoca.h>
#include <kservicetypeprofile.h>
#include <kstandarddirs.h>
#include <klocale.h>

#ifdef Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

FileTypeDialog::FileTypeDialog( KMimeType::Ptr mime )
  : KDialogBase( 0L, 0, false, QString(), /* Help | */ Cancel | Apply | Ok,
                 Ok, false )
{
  init( mime, false );
}

FileTypeDialog::FileTypeDialog( KMimeType::Ptr mime, bool newItem )
  : KDialogBase( 0L, 0, false, QString(), /* Help | */ Cancel | Apply | Ok,
                 Ok, false )
{
  init( mime, newItem );
}

void FileTypeDialog::init( KMimeType::Ptr mime, bool newItem )
{
  m_details = new FileTypeDetails( this );
  Q3ListView * dummyListView = new Q3ListView( m_details );
  dummyListView->hide();
  m_item = new TypesListItem( dummyListView, mime, newItem );
  m_details->setTypeItem( m_item );

  // This code is very similar to kcdialog.cpp
  setMainWidget( m_details );
  connect(m_details, SIGNAL(changed(bool)), this, SLOT(clientChanged(bool)));
  // TODO setHelp()
  enableButton(Apply, false);

  connect( KSycoca::self(), SIGNAL( databaseChanged() ), SLOT( slotDatabaseChanged() ) );
}

void FileTypeDialog::save()
{
  if (m_item->isDirty()) {
    m_item->sync();
    KService::rebuildKSycoca(this);
  }
}

void FileTypeDialog::slotApply()
{
  save();
}

void FileTypeDialog::slotOk()
{
  save();
  accept();
}

void FileTypeDialog::clientChanged(bool state)
{
  // enable/disable buttons
  enableButton(User1, state);
  enableButton(Apply, state);
}

void FileTypeDialog::slotDatabaseChanged()
{
  if ( KSycoca::self()->isChanged( "mime" ) )
  {
      m_item->refresh();
  }
}

#include "keditfiletype.moc"

static KCmdLineOptions options[] =
{
  { "parent <winid>", I18N_NOOP("Makes the dialog transient for the window specified by winid"), 0 },
  { "+mimetype",  I18N_NOOP("File type to edit (e.g. text/html)"), 0 },
  KCmdLineLastOption
};

int main(int argc, char ** argv)
{
  KServiceTypeProfile::setConfigurationMode();
  KLocale::setMainCatalog("filetypes");
  KAboutData aboutData( "keditfiletype", I18N_NOOP("KEditFileType"), "1.0",
                        I18N_NOOP("KDE file type editor - simplified version for editing a single file type"),
                        KAboutData::License_GPL,
                        I18N_NOOP("(c) 2000, KDE developers") );
  aboutData.addAuthor("Preston Brown",0, "pbrown@kde.org");
  aboutData.addAuthor("David Faure",0, "faure@kde.org");

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
  KApplication app;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (args->count() == 0)
    KCmdLineArgs::usage();

  QString arg = args->arg(0);

  bool createType = arg.startsWith( "*" );

  KMimeType::Ptr mime;
  
  if ( createType ) {
    QString mimeString = "application/x-kdeuser%1";
    QString loc;
    int inc = 0;
    do {
      ++inc;
      loc = locateLocal( "mime", mimeString.arg( inc ) + ".desktop" );
    }
    while ( QFile::exists( loc ) );

    QStringList patterns;
    if ( arg.length() > 2 )
	patterns << arg.toLower() << arg.toUpper();
    QString comment;
    if ( arg.startsWith( "*." ) && arg.length() >= 3 ) {
	QString type = arg.mid( 3 ).prepend( arg[2].toUpper() );
        comment = i18n( "%1 File", type );
    }
    mime = new KMimeType( loc, mimeString.arg( inc ), QString(), comment, patterns );
  }
  else { 
    mime = KMimeType::mimeType( arg );
    if (!mime)
      kFatal() << "Mimetype " << arg << " not found" << endl;
  }

  FileTypeDialog dlg( mime, createType );
#if defined Q_WS_X11
  if( args->isSet( "parent" )) {
    bool ok;
    long id = QString(args->getOption("parent")).toLong(&ok);
    if (ok)
      XSetTransientForHint( QX11Info::display(), dlg.winId(), id );
  }
#endif
  args->clear();
  if ( !createType )
    dlg.setCaption( i18n("Edit File Type %1", mime->name()) );
  else {
    dlg.setCaption( i18n("Create New File Type %1", mime->name()) );
    dlg.enableButton( KDialogBase::Apply, true );
  }
  app.setMainWidget( &dlg );
  dlg.show(); // non-modal

  return app.exec();
}

