/*
 *  Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "konqueror.h"

#include <dcopref.h>
#include <kconfig.h>

#include <QRadioButton>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <klocale.h>

namespace KCMPerformance
{

Konqueror::Konqueror( QWidget* parent_P )
    : Konqueror_ui( parent_P )
    {
    rb_never_reuse->setWhatsThis(
        i18n( "Disables the minimization of memory usage and allows you "
              "to make each browsing activity independent from the others" ));
    rb_file_browsing_reuse->setWhatsThis(
        i18n( "With this option activated, only one instance of Konqueror "
              "used for file browsing will exist in the memory of your computer "
              "at any moment, "
              "no matter how many file browsing windows you open, "
              "thus reducing resource requirements."
              "<p>Be aware that this also means that, if something goes wrong, "
              "all your file browsing windows will be closed simultaneously" ));
    rb_always_reuse->setWhatsThis(
        i18n( "With this option activated, only one instance of Konqueror "
              "will exist in the memory of your computer at any moment, "
              "no matter how many browsing windows you open, "
              "thus reducing resource requirements."
              "<p>Be aware that this also means that, if something goes wrong, "
              "all your browsing windows will be closed simultaneously." ));
    connect( rb_never_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    connect( rb_file_browsing_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    connect( rb_always_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    rb_file_browsing_reuse->setChecked( true );

    QString tmp =
        i18n( "If non-zero, this option allows keeping Konqueror instances "
              "in memory after all their windows have been closed, up to the "
              "number specified in this option."
              "<p>When a new Konqueror instance is needed, one of these preloaded "
              "instances will be reused instead, improving responsiveness at "
              "the expense of the memory required by the preloaded instances." );
    sb_preload_count->setWhatsThis( tmp );
    lb_preload_count->setWhatsThis( tmp );
    cb_preload_on_startup->setWhatsThis(
        i18n( "If enabled, an instance of Konqueror will be preloaded after the ordinary KDE "
              "startup sequence."
              "<p>This will make the first Konqueror window open faster, but "
              "at the expense of longer KDE startup times (but you will be able to work "
              "while it is loading, so you may not even notice that it is taking longer)." ));
    cb_always_have_preloaded->setWhatsThis(
        i18n( "If enabled, KDE will always try to have one preloaded Konqueror instance ready; "
              "preloading a new instance in the background whenever there is not one available, "
              "so that windows will always open quickly."
              "<p><b>Warning:</b> In some cases, it is actually possible that this will "
              "reduce perceived performance." ));
    connect( sb_preload_count, SIGNAL( valueChanged( int )), SLOT( preload_count_changed( int )));
    connect( sb_preload_count, SIGNAL( valueChanged( int )), SIGNAL( changed()));
    connect( cb_preload_on_startup, SIGNAL( clicked()), SIGNAL( changed()));
    connect( cb_always_have_preloaded, SIGNAL( clicked()), SIGNAL( changed()));
    defaults();
    }

void Konqueror::preload_count_changed( int count )
    {
    cb_preload_on_startup->setEnabled( count >= 1 );
    // forcing preloading with count == 1 can often do more harm than good, because
    // if there's one konqy preloaded, and the user requests "starting" new konqueror,
    // the preloaded instance will be used, new one will be preloaded, and if the user soon
    // "quits" konqueror, one of the instances will have to be terminated
    cb_always_have_preloaded->setEnabled( count >= 2 );
    }

void Konqueror::load()
    {
    KConfig cfg( "konquerorrc", true );
    cfg.setGroup( "Reusing" );
    allowed_parts = cfg.readEntry( "SafeParts", "SAFE" );
    if( allowed_parts == "ALL" )
        rb_always_reuse->setChecked( true );
    else if( allowed_parts.isEmpty())
        rb_never_reuse->setChecked( true );
    else
        rb_file_browsing_reuse->setChecked( true );
    sb_preload_count->setValue( cfg.readEntry( "MaxPreloadCount", 1 ));
    cb_always_have_preloaded->setChecked( cfg.readEntry( "AlwaysHavePreloaded", QVariant(false )).toBool());
    cb_preload_on_startup->setChecked( cfg.readEntry( "PreloadOnStartup", QVariant(false )).toBool());
    }

void Konqueror::save()
    {
    KConfig cfg( "konquerorrc" );
    cfg.setGroup( "Reusing" );
    if( rb_always_reuse->isChecked())
        allowed_parts = "ALL";
    else if( rb_never_reuse->isChecked())
        allowed_parts = "";
    else
        {
        if( allowed_parts.isEmpty() || allowed_parts == "ALL" )
            allowed_parts = "SAFE";
        // else - keep allowed_parts as read from the file, as the user may have modified the list there
        }
    cfg.writeEntry( "SafeParts", allowed_parts );
    int count = sb_preload_count->value();
    cfg.writeEntry( "MaxPreloadCount", count );
    cfg.writeEntry( "PreloadOnStartup", cb_preload_on_startup->isChecked() && count >= 1 );
    cfg.writeEntry( "AlwaysHavePreloaded", cb_always_have_preloaded->isChecked() && count >= 2 );
    cfg.sync();
    DCOPRef ref1( "konqueror*", "KonquerorIface" );
    ref1.send( "reparseConfiguration()" );
    DCOPRef ref2( "kded", "konqy_preloader" );
    ref2.send( "reconfigure()" );
    }

void Konqueror::defaults()
    {
    rb_file_browsing_reuse->setChecked( true );
    allowed_parts = "SAFE";
    sb_preload_count->setValue( 1 );
    cb_preload_on_startup->setChecked( false );
    cb_always_have_preloaded->setChecked( false );
    preload_count_changed( sb_preload_count->value());
    }

} // namespace

#include "konqueror.moc"
