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
 * KFind (c) 1998-2000 The KDE Developers
  Martin Hartig
  Stephan Kulow <coolo@kde.org>
  Mario Weilguni <mweilguni@sime.com>
  Alex Zepeda <jazepeda@pacbell.net>
  Miroslav Flídr <flidr@kky.zcu.cz>
  Harri Porten <porten@kde.org>
  Dima Rogozin <dima@mercury.co.il>
  Carsten Pfeiffer <pfeiffer@kde.org>
  Hans Petter Bieker <bieker@kde.org>
  Waldo Bastian <bastian@kde.org>
  Eric Coquelle <coquelle@caramail.com>

 **********************************************************************/

#include <qpushbutton.h>
#include <qlayout.h>
#include <qvbox.h>

#include <kdialog.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kfileitem.h>
#include <kseparator.h>
//#include <kstatusbar.h>

#include "kftabdlg.h"
#include "kquery.h"

#include "kfind.h"
#include "kfind.moc"

Kfind::Kfind(QWidget *parent, const char *name)
  : QWidget( parent, name )
{
  kdDebug() << "Kfind::Kfind " << this << endl;
  QBoxLayout * mTopLayout = new QBoxLayout( this, QBoxLayout::LeftToRight,
                                            KDialog::marginHint(), KDialog::spacingHint() );

  // create tabwidget
  tabWidget = new KfindTabWidget( this );
  mTopLayout->addWidget(tabWidget);

  // create separator
  KSeparator * mActionSep = new KSeparator( this );
  mActionSep->setFocusPolicy( QWidget::ClickFocus );
  mActionSep->setOrientation( QFrame::VLine );
  mTopLayout->addWidget(mActionSep);

  // create button box
  QWidget * mButtonBox = new QVBox( this );
  mTopLayout->addWidget(mButtonBox);

  mSearch = new QPushButton( i18n("&Find"), mButtonBox );
  connect( mSearch, SIGNAL(clicked()), this, SLOT( startSearch() ) );
  mStop = new QPushButton( i18n("Stop"), mButtonBox );
  connect( mStop, SIGNAL(clicked()), this, SLOT( stopSearch() ) );
  mSave = new QPushButton( i18n("Save..."), mButtonBox );
  connect( mSave, SIGNAL(clicked()), this, SLOT( saveResults() ) );

  QPushButton * mClose = new QPushButton( i18n("&Close"), mButtonBox );
  connect( mClose, SIGNAL(clicked()), this, SIGNAL( destroyMe() ) );

  mSearch->setEnabled(true); // Enable "Search"
  mStop->setEnabled(false);  // Disable "Stop"
  mSave->setEnabled(false);  // Disable "Save..."
}

Kfind::~Kfind()
{
  kdDebug() << "Kfind::~Kfind" << endl;
}

void Kfind::setURL( const KURL &url )
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

  query->start();
}

void Kfind::stopSearch()
{
  // will call KFindPart::slotResult, which calls searchFinished here
  query->kill();
}

/*
void Kfind::newSearch()
{
  // WABA: Not used any longer?
  stopSearch();

  tabWidget->setDefaults();

  emit haveResults(false);
  emit resultSelected(false);

  setFocus();
}
*/

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
