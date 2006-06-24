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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
//
// File previews configuration
//

#include <QCheckBox>
#include <QLabel>
#include <QLayout>

//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <dbus/qdbus.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kglobal.h>
#include <k3listview.h>
#include <klocale.h>
#include <knuminput.h>
#include <kprotocolmanager.h>

#include "previews.h"

//-----------------------------------------------------------------------------

class PreviewCheckListItem : public Q3CheckListItem
{
  public:
    PreviewCheckListItem( Q3ListView *parent, const QString &text )
      : Q3CheckListItem( parent, text, CheckBoxController )
    {}

    PreviewCheckListItem( Q3ListViewItem *parent, const QString &text )
      : Q3CheckListItem( parent, text, CheckBox )
    {}

  protected:
    void stateChange( bool )
    {
        static_cast<KPreviewOptions *>( listView()->parent() )->changed();
    }
};

KPreviewOptions::KPreviewOptions( KInstance *inst, QWidget *parent )
    : KCModule( inst, parent )
{
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setMargin(0);
    lay->setSpacing(KDialog::spacingHint());

    lay->addWidget( new QLabel( i18n("<p>Allow previews, \"Folder Icons Reflect Contents\", and "
                                     "retrieval of meta-data on protocols:</p>"), this ) );

    setQuickHelp( i18n("<h1>Preview Options</h1> Here you can modify the behavior "
                "of Konqueror when it shows the files in a folder."
                "<h2>The list of protocols:</h2> check the protocols over which "
                "previews should be shown; uncheck those over which they should not. "
                "For instance, you might want to show previews over SMB if the local "
                "network is fast enough, but you might disable it for FTP if you often "
                "visit very slow FTP sites with large images."
                "<h2>Maximum File Size:</h2> select the maximum file size for which "
                "previews should be generated. For instance, if set to 1 MB (the default), "
                "no preview will be generated for files bigger than 1 MB, for speed reasons."));

    // Listview containing checkboxes for all protocols that support listing
    K3ListView *listView = new K3ListView( this );
    listView->addColumn( i18n( "Select Protocols" ) );
    listView->setFullWidth( true );

    QHBoxLayout *hbox = new QHBoxLayout();
    lay->addItem( hbox );
    hbox->addWidget( listView );
    hbox->addStretch();

    PreviewCheckListItem *localItems = new PreviewCheckListItem( listView,
        i18n( "Local Protocols" ) );
    PreviewCheckListItem *inetItems = new PreviewCheckListItem( listView,
        i18n( "Internet Protocols" ) );

    QStringList protocolList = KProtocolInfo::protocols();
    protocolList.sort();
    QStringList::Iterator it = protocolList.begin();

    KUrl url;
    url.setPath("/");

    for ( ; it != protocolList.end() ; ++it )
    {
        url.setProtocol( *it );
        if ( KProtocolManager::supportsListing( url ) )
        {
            Q3CheckListItem *item;
            if ( KProtocolInfo::protocolClass( *it ) == ":local" )
                item = new PreviewCheckListItem( localItems, ( *it ) );
            else
                item = new PreviewCheckListItem( inetItems, ( *it ) );

            m_items.append( item );
        }
    }

    listView->setOpen( localItems, true );
    listView->setOpen( inetItems, true );

    listView->setWhatsThis(
                     i18n("This option makes it possible to choose when the file previews, "
                          "smart folder icons, and meta-data in the File Manager should be activated.\n"
                          "In the list of protocols that appear, select which ones are fast "
                          "enough for you to allow previews to be generated.") );

    QLabel *label = new QLabel( i18n( "&Maximum file size:" ), this );
    lay->addWidget( label );

    m_maxSize = new KDoubleNumInput( this );
    m_maxSize->setSuffix( i18n(" MB") );
    m_maxSize->setRange( 0.02, 10, 0.02, true );
    m_maxSize->setPrecision( 1 );
    label->setBuddy( m_maxSize );
    lay->addWidget( m_maxSize );
    connect( m_maxSize, SIGNAL( valueChanged(double) ), SLOT( changed() ) );

    m_boostSize = new QCheckBox(i18n("&Increase size of previews relative to icons"), this);
    connect( m_boostSize, SIGNAL( toggled(bool) ), SLOT( changed() ) );
    lay->addWidget(m_boostSize);

    m_useFileThumbnails = new QCheckBox(i18n("&Use thumbnails embedded in files"), this);
    connect( m_useFileThumbnails, SIGNAL( toggled(bool) ), SLOT( changed() ) );

    lay->addWidget(m_useFileThumbnails);

    m_useFileThumbnails->setWhatsThis(
                i18n("Select this to use thumbnails that are found inside some "
                "file types (e.g. JPEG). This will increase speed and reduce "
                "disk usage. Deselect it if you have files that have been processed "
                "by programs which create inaccurate thumbnails, such as ImageMagick.") );

    lay->addWidget( new QWidget(this), 10 );

    load();
}

// Default: 1 MB
#define DEFAULT_MAXSIZE (1024*1024)

void KPreviewOptions::load(bool useDefaults)
{
    // *** load and apply to GUI ***
    KGlobal::config()->setReadDefaults(useDefaults);
    KConfigGroup group( KGlobal::config(), "PreviewSettings" );
    Q3PtrListIterator<Q3CheckListItem> it( m_items );

    for ( ; it.current() ; ++it ) {
        QString protocol( it.current()->text() );
        if ( ( protocol == "file" ) && ( !group.hasKey ( protocol ) ) )
          // file should be enabled in case is not defined because if not so
          // than preview's lost when size is changed from default one
          it.current()->setOn( true );
        else
          it.current()->setOn( group.readEntry( protocol, QVariant(false )).toBool() );
    }
    // config key is in bytes (default value 1MB), numinput is in MB
    m_maxSize->setValue( ((double)group.readEntry( "MaximumSize", DEFAULT_MAXSIZE )) / (1024*1024) );

    m_boostSize->setChecked( group.readEntry( "BoostSize", QVariant(false /*default*/ )).toBool() );
    m_useFileThumbnails->setChecked( group.readEntry( "UseFileThumbnails", QVariant(true /*default*/ )).toBool() );
    KGlobal::config()->setReadDefaults(false);
}

void KPreviewOptions::load()
{
    load(false);
}

void KPreviewOptions::defaults()
{
    load(true);
}

void KPreviewOptions::save()
{
    KConfigGroup group( KGlobal::config(), "PreviewSettings" );
    Q3PtrListIterator<Q3CheckListItem> it( m_items );
    for ( ; it.current() ; ++it ) {
        QString protocol( it.current()->text() );
        group.writeEntry( protocol, it.current()->isOn(), KConfigBase::Normal | KConfigBase::Global );
    }
    // config key is in bytes, numinput is in MB
    group.writeEntry( "MaximumSize", qRound( m_maxSize->value() *1024*1024 ), KConfigBase::Normal | KConfigBase::Global );
    group.writeEntry( "BoostSize", m_boostSize->isChecked(), KConfigBase::Normal | KConfigBase::Global );
    group.writeEntry( "UseFileThumbnails", m_useFileThumbnails->isChecked(), KConfigBase::Normal | KConfigBase::Global );
    group.sync();

    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::signal("/Konqueror", "org.kde.Konqueror", "reparseConfiguration");
    QDBus::sessionBus().send(message);
}

void KPreviewOptions::changed()
{
    emit KCModule::changed(true);
}

#include "previews.moc"
