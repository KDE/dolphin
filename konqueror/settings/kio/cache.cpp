/*
   cache.cpp - Proxy configuration dialog

   Copyright (C) 2001,02,03 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QLabel>
#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
//Added by qt3to4:
#include <QVBoxLayout>

#include <kdebug.h>
#include <klocale.h>
#include <kprocess.h>
#include <knuminput.h>

#include "ksaveioconfig.h"
#include <kio/http_slave_defaults.h>

#include "cache.h"
#include "ui_cache_ui.h"
#include <kgenericfactory.h>

typedef KGenericFactory<KCacheConfigDialog> KCacheConfigDialogFactory;
K_EXPORT_COMPONENT_FACTORY(cache, KCacheConfigDialogFactory("kcmkio"))

KCacheConfigDialog::KCacheConfigDialog(QWidget *parent, const QStringList &)
    : KCModule(KCacheConfigDialogFactory::instance(), parent)
{
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);
  m_dlg = new CacheDlgUI(this);
  mainLayout->addWidget(m_dlg);
  mainLayout->addStretch();

  load();
}

void KCacheConfigDialog::load()
{
  m_dlg->cbUseCache->setChecked(KProtocolManager::useCache());
  m_dlg->sbMaxCacheSize->setValue( KProtocolManager::maxCacheSize() );

  KIO::CacheControl cc = KProtocolManager::cacheControl();

  if (cc==KIO::CC_Verify)
      m_dlg->rbVerifyCache->setChecked( true );
  else if (cc==KIO::CC_Refresh)
      m_dlg->rbVerifyCache->setChecked( true );
  else if (cc==KIO::CC_CacheOnly)
      m_dlg->rbOfflineMode->setChecked( true );
  else if (cc==KIO::CC_Cache)
      m_dlg->rbCacheIfPossible->setChecked( true );

  // Config changed notifications...
  connect ( m_dlg->cbUseCache, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( m_dlg->rbVerifyCache, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( m_dlg->rbOfflineMode, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( m_dlg->rbCacheIfPossible, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( m_dlg->sbMaxCacheSize, SIGNAL(valueChanged(int)), SLOT(configChanged()) );
  connect ( m_dlg->pbClearCache, SIGNAL(clicked()), SLOT(slotClearCache()) );
  emit changed( false );
} 

void KCacheConfigDialog::save()
{
  KSaveIOConfig::setUseCache( m_dlg->cbUseCache->isChecked() );
  KSaveIOConfig::setMaxCacheSize( m_dlg->sbMaxCacheSize->value() );

  if ( !m_dlg->cbUseCache->isChecked() )
      KSaveIOConfig::setCacheControl(KIO::CC_Refresh);
  else if ( m_dlg->rbVerifyCache->isChecked() )
      KSaveIOConfig::setCacheControl(KIO::CC_Refresh);
  else if ( m_dlg->rbOfflineMode->isChecked() )
      KSaveIOConfig::setCacheControl(KIO::CC_CacheOnly);
  else if ( m_dlg->rbCacheIfPossible->isChecked() )
      KSaveIOConfig::setCacheControl(KIO::CC_Cache);

  // Update running io-slaves...
  KSaveIOConfig::updateRunningIOSlaves (this);

  emit changed( false );
}

void KCacheConfigDialog::defaults()
{
  m_dlg->cbUseCache->setChecked( true );
  m_dlg->rbVerifyCache->setChecked( true );
  m_dlg->sbMaxCacheSize->setValue( DEFAULT_MAX_CACHE_SIZE );

  emit changed( true );
}

QString KCacheConfigDialog::quickHelp() const
{
  return i18n( "<h1>Cache</h1><p>This module lets you configure your cache settings.</p>"
                "<p>The cache is an internal memory in Konqueror where recently "
                "read web pages are stored. If you want to retrieve a web "
                "page again that you have recently read, it will not be "
                "downloaded from the Internet, but rather retrieved from the "
                "cache, which is a lot faster.</p>" );
}

void KCacheConfigDialog::configChanged()
{
  emit changed( true );
}

void KCacheConfigDialog::slotClearCache()
{
  KProcess process;
  process << "kio_http_cache_cleaner" << "--clear-all";
  process.start(KProcess::DontCare);
  // Cleaning up might take a while. Better detach.
  process.detach();
}

#include "cache.moc"
