/* This file is part of the KDE libraries
    Copyright (C) 2002 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
//
// File previews configuration
//

#include <qcheckbox.h>
#include <qscrollview.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qwhatsthis.h>

#include "previews.h"

#include <kprotocolinfo.h>
#include <kdialog.h>
#include <konq_defaults.h> // include default values directly from libkonq
#include <klocale.h>
#include <kglobal.h>
#include <knuminput.h>
#include <kconfig.h>

//-----------------------------------------------------------------------------

KPreviewOptions::KPreviewOptions( QWidget *parent, const char *name )
    : KCModule( parent, "kcmkonq" )
{
    QVBoxLayout *lay = new QVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());

    lay->addWidget( new QLabel( i18n("Allow previews and \"Folder Icons Reflect Contents\" on protocols:"), this ) );

    QHBoxLayout *hbox = new QHBoxLayout(); // hbox to avoid a very wide scrollview
    lay->addLayout( hbox );

    // Scrollview containing checkboxes for all protocols that support listing
    QScrollView * scrollView = new QScrollView( this );
    scrollView->setResizePolicy( QScrollView::AutoOneFit );
    scrollView->setMaximumHeight( 400 ); // don't let the dialog become huge
    //lay->addWidget( scrollView );
    hbox->addWidget( scrollView, 1 );

    hbox->addWidget( new QWidget( this ), 1 );

    QVBox *box = new QVBox( scrollView->viewport() );
    scrollView->addChild( box );

    QStringList protocolList = KProtocolInfo::protocols();
    protocolList.sort();
    QStringList::Iterator it = protocolList.begin();
    for ( ; it != protocolList.end() ; ++it )
    {
        //KURL testURL ( (*it) + "://host/" );
        if ( KProtocolInfo::supportsListing( *it ) )
        {
            QCheckBox * cb = new QCheckBox( (*it), box, (*it).latin1() /*name*/ );
            connect( cb, SIGNAL( toggled(bool) ), SLOT( changed() ) );
            m_boxes.append( cb );
        }
    }
    QWhatsThis::add( scrollView,
                     i18n("This option makes it possible to choose when the file previews "
                          "and smart folder icons in the File Manager should be activated.\n"
                          "In the list of protocols that appear, select which ones are fast "
                          "enough for you to allow previews to be generated.") );

    lay->addWidget( new QLabel( i18n( "Maximum file size:" ), this ) );

    m_maxSize = new KDoubleNumInput( this );
    m_maxSize->setSuffix( i18n(" MB") );
    m_maxSize->setRange( 0.2, 100, 0.2, true );
    m_maxSize->setPrecision( 1 );
    lay->addWidget( m_maxSize );
    connect( m_maxSize, SIGNAL( valueChanged(double) ), SLOT( changed() ) );

    m_boostSize = new QCheckBox(i18n("Increase size of previews relative to icons"), this);
    lay->addWidget(m_boostSize);

    m_useFileThumbnails = new QCheckBox(i18n("Use thumbnails embedded in files"), this);
    lay->addWidget(m_useFileThumbnails);

    QWhatsThis::add( m_useFileThumbnails,
                i18n("Select this to use thumbnails that are found inside some "
                "file types (e.g. JPEG). This will increase speed and reduce "
                "disk usage. Deselect it if you have files that have been processed "
                "by programs which create inaccurate thumbnails, such as ImageMagick.") );    
    
    lay->addWidget( new QWidget(this), 10 );

    load();
}

// Default: 1 MB
#define DEFAULT_MAXSIZE (1024*1024)

void KPreviewOptions::load()
{
    // *** load and apply to GUI ***
    KConfigGroup group( KGlobal::config(), "PreviewSettings" );
    QPtrListIterator<QCheckBox> it( m_boxes );
    for ( ; it.current() ; ++it ) {
        QString protocol( it.current()->name() );
        it.current()->setChecked( group.readBoolEntry( protocol, true /*default*/ ) );
    }
    // config key is in bytes (default value 1MB), numinput is in MB
    m_maxSize->setValue( ((double)group.readNumEntry( "MaximumSize", DEFAULT_MAXSIZE )) / (1024*1024) );

    m_boostSize->setChecked( group.readBoolEntry( "BoostSize", true /*default*/ ) );
    m_useFileThumbnails->setChecked( group.readBoolEntry( "UseFileThumbnails", true /*default*/ ) );
}

void KPreviewOptions::defaults()
{
    QPtrListIterator<QCheckBox> it( m_boxes );
    for ( ; it.current() ; ++it ) {
        it.current()->setChecked( true /*default*/ );
    }
    m_maxSize->setValue( DEFAULT_MAXSIZE / (1024*1024) );
    m_boostSize->setChecked( true );
}

void KPreviewOptions::save()
{
    KConfigGroup group( KGlobal::config(), "PreviewSettings" );
    QPtrListIterator<QCheckBox> it( m_boxes );
    for ( ; it.current() ; ++it ) {
        QString protocol( it.current()->name() );
        group.writeEntry( protocol, it.current()->isChecked(), true, true );
    }
    // config key is in bytes, numinput is in MB
    group.writeEntry( "MaximumSize", qRound( m_maxSize->value() *1024*1024 ), true, true );
    group.writeEntry( "BoostSize", m_boostSize->isChecked(), true, true );
    group.writeEntry( "UseFileThumbnails", m_useFileThumbnails->isChecked(), true, true );
    group.sync();
}

QString KPreviewOptions::quickHelp() const
{
    return i18n("<h1>Preview Options</h1> Here you can modify the behavior "
                "of Konqueror when it shows the files in a directory."
                "<h2>The list of protocols:</h2> check the protocols over which "
                "previews should be shown, uncheck those over which they shouldn't. "
                "For instance, you might want to show previews over SMB if the local "
                "network is fast enough, but you might disable it for FTP if you often "
                "visit very slow FTP sites with large images."
                "<h2>Maximum File Size:</h2> select the maximum file size for which "
                "previews should be generated. For instance, if set to 1 MB (the default), "
                "no preview will be generated for files bigger than 1 MB, for speed reasons.");
}

void KPreviewOptions::changed()
{
    emit KCModule::changed(true);
}

#include "previews.moc"
