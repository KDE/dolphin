/***********************************************************************
 *
 *  Kfind.cpp
 *
 * This is KFind, released under GPL
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 * KFind (c) 1998-2003 The KDE Developers
  Martin Hartig
  Stephan Kulow <coolo@kde.org>
  Mario Weilguni <mweilguni@sime.com>
  Alex Zepeda <zipzippy@sonic.net>
  Miroslav Flídr <flidr@kky.zcu.cz>
  Harri Porten <porten@kde.org>
  Dima Rogozin <dima@mercury.co.il>
  Carsten Pfeiffer <pfeiffer@kde.org>
  Hans Petter Bieker <bieker@kde.org>
  Waldo Bastian <bastian@kde.org>
  Beppe Grimaldi <grimalkin@ciaoweb.it>
  Eric Coquelle <coquelle@caramail.com>

 **********************************************************************/

#include <QLayout>
#include <QLineEdit>
#include <QCheckBox>

#include <kpushbutton.h>
#include <kvbox.h>
#include <kdialog.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstdguiitem.h>
#include <kdirlister.h>

#include "kftabdlg.h"
#include "kquery.h"

#include "kfind.moc"

Kfind::Kfind(QWidget *parent)
  : QWidget( parent )
{
  kDebug() << "Kfind::Kfind " << this << endl;
  QBoxLayout * mTopLayout = new QBoxLayout( QBoxLayout::LeftToRight, this );
  mTopLayout->setMargin( KDialog::marginHint() );
  mTopLayout->setSpacing( KDialog::spacingHint() );

  // create tabwidget
  tabWidget = new KfindTabWidget( this );
  mTopLayout->addWidget(tabWidget);

  // create button box
  KVBox * mButtonBox = new KVBox( this );
  QVBoxLayout *lay = (QVBoxLayout*)mButtonBox->layout();
  lay->addStretch(1);
  mTopLayout->addWidget(mButtonBox);

  mSearch = new KPushButton( KStdGuiItem::find(), mButtonBox );
  mButtonBox->setSpacing( (tabWidget->sizeHint().height()-4*mSearch->sizeHint().height()) / 4);
  connect( mSearch, SIGNAL(clicked()), this, SLOT( startSearch() ) );
  mStop = new KPushButton( KStdGuiItem::stop(), mButtonBox );
  connect( mStop, SIGNAL(clicked()), this, SLOT( stopSearch() ) );
  mSave = new KPushButton( KStdGuiItem::saveAs(), mButtonBox );
  connect( mSave, SIGNAL(clicked()), this, SLOT( saveResults() ) );

  KPushButton * mClose = new KPushButton( KStdGuiItem::close(), mButtonBox );
  connect( mClose, SIGNAL(clicked()), this, SIGNAL( destroyMe() ) );

  // react to search requests from widget
  connect( tabWidget, SIGNAL(startSearch()), this, SLOT( startSearch() ) );

  mSearch->setEnabled(true); // Enable "Search"
  mStop->setEnabled(false);  // Disable "Stop"
  mSave->setEnabled(false);  // Disable "Save..."

  dirlister=new KDirLister();
}

Kfind::~Kfind()
{
  stopSearch();
  dirlister->stop();
  delete dirlister;
  kDebug() << "Kfind::~Kfind" << endl;
}

void Kfind::setURL( const KUrl &url )
{
  tabWidget->setURL( url );
}

void Kfind::startSearch()
{
  tabWidget->setQuery(query);
  emit started();

  //emit resultSelected(false);
  //emit haveResults(false);

  mSearch->setEnabled(false); // Disable "Search"
  mStop->setEnabled(true);  // Enable "Stop"
  mSave->setEnabled(false);  // Disable "Save..."

  tabWidget->beginSearch();

  dirlister->openURL(KUrl(tabWidget->dirBox->currentText().trimmed()));

  query->start();
}

void Kfind::stopSearch()
{
  // will call KFindPart::slotResult, which calls searchFinished here
  query->kill();
}

void Kfind::searchFinished()
{
  mSearch->setEnabled(true); // Enable "Search"
  mStop->setEnabled(false);  // Disable "Stop"
  // ## TODO mSave->setEnabled(true);  // Enable "Save..."

  tabWidget->endSearch();
  setFocus();
}


void Kfind::saveResults()
{
  // TODO
}

void Kfind::setFocus()
{
  tabWidget->setFocus();
}

void Kfind::saveState( QDataStream *stream )
{
  query->kill();
  *stream << tabWidget->nameBox->currentText();
  *stream << tabWidget->dirBox->currentText();
  *stream << tabWidget->typeBox->currentIndex();
  *stream << tabWidget->textEdit->text();
  *stream << (int)( tabWidget->subdirsCb->isChecked() ? 0 : 1 );
}

void Kfind::restoreState( QDataStream *stream )
{
  QString namesearched, dirsearched,containing;
  int typeIdx;
  int subdirs;
  *stream >> namesearched;
  *stream >> dirsearched;
  *stream >> typeIdx;
  *stream >> containing;
  *stream >> subdirs;
  tabWidget->nameBox->addItem( namesearched, 0);
  tabWidget->dirBox->addItem ( dirsearched, 0);
  tabWidget->typeBox->setCurrentIndex(typeIdx);
  tabWidget->textEdit->setText ( containing );
  tabWidget->subdirsCb->setChecked( ( subdirs==0 ? true : false ));
}
