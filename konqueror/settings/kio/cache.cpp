/*
   cache.cpp - Proxy configuration dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <dcopclient.h>
#include <ksaveioconfig.h>
#include <kio/http_slave_defaults.h>

#include "cache.h"

KCacheConfigDialog::KCacheConfigDialog( QWidget* parent, const char* name )
                   :KCModule( parent, name )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this, KDialog::marginHint(),
                                               KDialog::spacingHint() );
    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    cb_useCache = new QCheckBox( i18n("&Use cache"), this, "cb_useCache" );
    cb_useCache->setSizePolicy( QSizePolicy(QSizePolicy::Fixed,
                                            QSizePolicy::Fixed,
                                            cb_useCache->sizePolicy().hasHeightForWidth()) );
    
    QWhatsThis::add( cb_useCache, i18n("Click here if you want the web pages "
                                       "you view to be stored on your hard "
                                       "disk for quicker access. Enabling "
                                       "this feature will make browsing "
                                       "faster, since the pages will only be "
                                       "downloaded as necessary. This is "
                                       "especially true if you have a slow "
                                       "connection to the Internet.") );
    hlay->addWidget( cb_useCache );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                                           QSizePolicy::Minimum );
    hlay->addItem( spacer );
    mainLayout->addLayout( hlay );

    gb_Cache_policy = new QButtonGroup( i18n("Policy"), this, "gb_Cache_policy" );
    gb_Cache_policy->setEnabled( false );
    gb_Cache_policy->setColumnLayout(0, Qt::Vertical );
    gb_Cache_policy->layout()->setSpacing( 0 );
    gb_Cache_policy->layout()->setMargin( 0 );

    QVBoxLayout* gb_Cache_policyLayout = new QVBoxLayout( gb_Cache_policy->layout() );
    gb_Cache_policyLayout->setAlignment( Qt::AlignTop );
    gb_Cache_policyLayout->setSpacing( KDialog::spacingHint() );
    gb_Cache_policyLayout->setMargin( KDialog::marginHint() );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    rb_verify = new QRadioButton( i18n("&Keep cache in sync"), gb_Cache_policy,
                                  "rb_verify" );
    
    QWhatsThis::add( rb_verify, i18n("Select this if you want to verify "
                                     "whether the page cached in your hard "
                                     "disk is still valid.") );
    hlay->addWidget( rb_verify );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    gb_Cache_policyLayout->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    rb_cacheIfPossible = new QRadioButton( i18n("Use cache if &possible"),
                                           gb_Cache_policy,
                                           "rb_cacheIfPossible" );
    
    QWhatsThis::add( rb_cacheIfPossible, i18n("Enable this to always lookup "
                                              "the cache before connecting "
                                              "to the Internet. You can still "
                                     "use the reload button to synchronize the "
                                     "cache with the remote host.") );
    hlay->addWidget( rb_cacheIfPossible );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    gb_Cache_policyLayout->addLayout( hlay );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );

    rb_offlineMode = new QRadioButton( i18n("O&ffline browsing mode"),
                                       gb_Cache_policy, "rb_offlineMode" );
    
    QWhatsThis::add( rb_offlineMode, i18n("Enable this to prevent HTTP "
                                          "requests by KDE applications "
                                          "by default.") );
    hlay->addWidget( rb_offlineMode );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    gb_Cache_policyLayout->addLayout( hlay );
    mainLayout->addWidget( gb_Cache_policy );

    hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( 0 );
    spacer = new QSpacerItem( 16, 20, QSizePolicy::Fixed,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );

    lb_max_cache_size = new QLabel( i18n("Disk cache &size:"), this,
                                    "lb_max_cache_size" );
    lb_max_cache_size->setEnabled( false );
    hlay->addWidget( lb_max_cache_size );

    sb_max_cache_size = new QSpinBox( this, "sb_max_cache_size" );
    sb_max_cache_size->setEnabled( false );
    sb_max_cache_size->setRange( 1, 999999 );
    sb_max_cache_size->setSuffix(i18n(" KB"));

    QWhatsThis::add( sb_max_cache_size, i18n("This is the average size "
                                             "in KB that the cache will "
                                             "take on your hard disk. Once "
                                             "in a while the oldest pages "
                                             "will be deleted from the cache "
                                             "to reduce it to this size.") );
    hlay->addWidget( sb_max_cache_size );

    pb_clearCache = new QPushButton( i18n("C&lear Cache"), this,
                                     "pb_clearCache" );
    pb_clearCache->setEnabled( false );
    QWhatsThis::add( pb_clearCache, i18n("Click this button to completely "
                                         "clear the HTTP cache. This can be "
                                         "sometimes useful to check if a "
                                         "wrong copy of a website has been "
                                         "cached, or to quickly free some "
                                         "disk space.") );
    hlay->addWidget( pb_clearCache );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    mainLayout->addLayout( hlay );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                              QSizePolicy::Expanding );
    mainLayout->addItem( spacer );

    // signals and slots connections
    connect( cb_useCache, SIGNAL( toggled(bool) ), gb_Cache_policy,
             SLOT( setEnabled(bool) ) );
    connect( cb_useCache, SIGNAL( toggled(bool) ), this,
             SLOT(configChanged()));
    connect( cb_useCache, SIGNAL( toggled(bool) ), lb_max_cache_size,
             SLOT( setEnabled(bool) ) );
    connect( cb_useCache, SIGNAL( toggled(bool) ), sb_max_cache_size,
             SLOT( setEnabled(bool) ) );
    connect( cb_useCache, SIGNAL( toggled(bool) ), pb_clearCache,
             SLOT( setEnabled(bool) ) );

    connect( gb_Cache_policy, SIGNAL(clicked ( int )), this,
             SLOT(configChanged()));
    connect( sb_max_cache_size, SIGNAL(valueChanged ( int )),this,
             SLOT(configChanged()));
    // buddies
    lb_max_cache_size->setBuddy( sb_max_cache_size );
    load();
}

KCacheConfigDialog::~KCacheConfigDialog()
{
}

void KCacheConfigDialog::load()
{
    cb_useCache->setChecked(KProtocolManager::useCache());

    KIO::CacheControl cc = KProtocolManager::cacheControl();

    if (cc==KIO::CC_Verify)
        rb_verify->setChecked( true );
    else if (cc==KIO::CC_CacheOnly)
        rb_offlineMode->setChecked(true);
    else if (cc==KIO::CC_Cache)
        rb_cacheIfPossible->setChecked(true);

    sb_max_cache_size->setValue( KProtocolManager::maxCacheSize() );

    bool useCache = cb_useCache->isChecked();
    gb_Cache_policy->setEnabled( useCache );
    lb_max_cache_size->setEnabled( useCache );
    sb_max_cache_size->setEnabled( useCache );
    pb_clearCache->setEnabled( useCache );
}

void KCacheConfigDialog::save()
{
    KSaveIOConfig::setUseCache( cb_useCache->isChecked() );
    KSaveIOConfig::setMaxCacheSize( sb_max_cache_size->value() );

    if ( !cb_useCache->isChecked() )
        KSaveIOConfig::setCacheControl(KIO::CC_Reload);
    else if ( rb_verify->isChecked() )
        KSaveIOConfig::setCacheControl(KIO::CC_Verify);
    else if ( rb_offlineMode->isChecked() )
        KSaveIOConfig::setCacheControl(KIO::CC_CacheOnly);
    else if ( rb_cacheIfPossible->isChecked() )
        KSaveIOConfig::setCacheControl(KIO::CC_Cache);

    // Update everyone...
    QByteArray data;
    QDataStream stream( data, IO_WriteOnly );
    stream << QString::null;
    
    KSaveIOConfig::updateRunningIOSlaves (this);
  
    emit changed( false );
}

void KCacheConfigDialog::defaults()
{
  cb_useCache->setChecked( true );
  rb_verify->setChecked( true );
  sb_max_cache_size->setValue( DEFAULT_MAX_CACHE_SIZE );
  
  emit changed( true );  
}

QString KCacheConfigDialog::quickHelp() const
{
    return i18n( "This module lets you configure your cache settings. The "
                 "cache is an internal memory in Konqueror where recently "
                 "read web pages are stored. If you want to retrieve a web "
                 "page again that you have recently read, it will not be "
                 "downloaded from the net, but rather retrieved from the "
                 "cache which is a lot faster." );
}

#include "cache.moc"
