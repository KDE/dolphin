/**
 *  Copyright (c) 2001 David Faure <david@mandrakesoft.com>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// Behaviour options for konqueror

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qslider.h>
#include <qwhatsthis.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <konq_defaults.h>
#include <kstddirs.h>
#include <kapp.h>
#include <dcopclient.h>
#include <kio/uiserver_stub.h>

#include "behaviour.h"

KBehaviourOptions::KBehaviourOptions(KConfig *config, QString group, QWidget *parent, const char *name )
    : KCModule(parent, name), g_pConfig(config), groupname(group)
{
    QLabel * label;
    int row = 0;

    QGridLayout *lay = new QGridLayout(this,10,4, // rows, cols
                                       KDialog::marginHint(),
                                       KDialog::spacingHint());     // border, space
    lay->setRowStretch(6,1);
    lay->setColStretch(1,1);
    lay->setColStretch(3,1);

    kfmclientConfig = new KConfig(QString::fromLatin1("kfmclientrc"), false, false);
    kfmclientConfig->setGroup(QString::fromLatin1("Settings")); //use these to set the one-process option in kfmclient

    row++;
    winPixmap = new QLabel(this);
    winPixmap->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    winPixmap->setPixmap(QPixmap(locate("data",
                                        "kcontrol/pics/onlyone.png")));
    winPixmap->setFixedSize( winPixmap->sizeHint() );
    lay->addMultiCellWidget(winPixmap, row, row, 1, 2);

    // ----
    row++;
    cbNewWin = new QCheckBox(i18n("&Open directories in separate windows"), this);
    QWhatsThis::add( cbNewWin, i18n("If this option is checked, Konqueror will open a new window when "
                                    "you open a directory, rather than showing that directory's contents in the current window."));
    lay->addMultiCellWidget(cbNewWin, row, row, 0, 2, Qt::AlignLeft);
    connect(cbNewWin, SIGNAL(clicked()), this, SLOT(changed()));
    connect(cbNewWin, SIGNAL(toggled(bool)), SLOT(updateWinPixmap(bool)));

    // ----
    row++;
    cbListProgress = new QCheckBox( i18n( "&Show network operations in a single window" ), this );
    lay->addMultiCellWidget(cbListProgress, row, row, 0, 2, Qt::AlignLeft);
    connect(cbListProgress, SIGNAL(clicked()), this, SLOT(changed()));

    QWhatsThis::add( cbListProgress, i18n("Checking this option will group the"
                                          " progress information for all network file transfers into a single window"
                                          " with a list. When the option is not checked, all transfers appear in a"
                                          " separate window.") );


    // ----
    row++;
    label = new QLabel(i18n("Home &URL:"), this);
    lay->addWidget(label, row, 0);

    homeURL = new QLineEdit(this);
    lay->addMultiCellWidget(homeURL, row, row, 1, 3);
    connect(homeURL, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
    label->setBuddy(homeURL);

    QString homestr = i18n("This is the URL (e.g. a directory or a web page) where "
                           "Konqueror will jump to when the \"Home\" button is pressed. Usually a 'tilde' (~).");
    QWhatsThis::add( label, homestr );
    QWhatsThis::add( homeURL, homestr );

    // ----
    row++;
    QString opstrg = i18n("With this option activated, only one instance of Konqueror "
                          "will exist in the memory of your computer at any moment, "
                          "no matter how many browsing windows you open, "
                          "thus reducing resource requirements."
                          "<p>Be aware that this also means that, if something goes wrong, "
                          "all your browsing windows will be closed simultaneously.");
    QString opstrl = i18n("With this option activated, only one instance of Konqueror "
                          "will exist in the memory of your computer at any moment, "
                          "no matter how many local browsing windows you open, "
                          "thus reducing resource requirements."
                          "<p>Be aware that this also means that, if something goes wrong, "
                          "all your local browsing windows will be closed simultaneously");
    QString opstrw = i18n("With this option activated, only one instance of Konqueror "
                          "will exist in the memory of your computer at any moment, "
                          "no matter how many web browsing windows you open, "
                          "thus reducing resource requirements."
                          "<p>Be aware that this also means that, if something goes wrong, "
                          "all your web browsing windows will be closed simultaneously");

    bgOneProcess = new QVButtonGroup(i18n("Minimize memory usage"), this);
    bgOneProcess->setExclusive( true );
    connect(bgOneProcess, SIGNAL(clicked(int)), this, SLOT(changed()));
    {
        rbOPNever = new QRadioButton(i18n("&Never"), bgOneProcess);
        QWhatsThis::add( rbOPNever, i18n("Disables the minimization of memory usage and allows you "
                                         "to make each browsing activity independent from the others"));

        rbOPLocal = new QRadioButton(i18n("For &local browsing only (recommended)"), bgOneProcess);
        QWhatsThis::add( rbOPLocal, opstrl);

        rbOPWeb = new QRadioButton(i18n("For &web browsing only"), bgOneProcess);
        QWhatsThis::add( rbOPWeb, opstrw);

        rbOPAlways = new QRadioButton(i18n("&Always (use with care)"), bgOneProcess);
        QWhatsThis::add( rbOPAlways, opstrg);

        rbOPLocal->setChecked(true);
    }

    lay->addMultiCellWidget(bgOneProcess, row, row, 0, 3);

    load();
}


void KBehaviourOptions::load()
{
    g_pConfig->setGroup( groupname );
    cbNewWin->setChecked( g_pConfig->readBoolEntry("AlwaysNewWin", false) );
    updateWinPixmap(cbNewWin->isChecked());

    homeURL->setText(g_pConfig->readEntry("HomeURL", "~"));

    QString val = kfmclientConfig->readEntry( QString::fromLatin1("StartNewKonqueror"),
                                              QString::fromLatin1("Web only") );
    if (val == QString::fromLatin1("Web only"))
        rbOPLocal->setChecked(true);
    else if (val == QString::fromLatin1("Local only"))
        rbOPWeb->setChecked(true);
    else if (val == QString::fromLatin1("Always") ||
             val == QString::fromLatin1("true") ||
             val == QString::fromLatin1("TRUE") ||
             val == QString::fromLatin1("1"))
        rbOPNever->setChecked(true);
    else
        rbOPAlways->setChecked(true);

    KConfig config("uiserverrc");
    config.setGroup( "UIServer" );

    cbListProgress->setChecked( config.readBoolEntry( "ShowList", false ) );
}

void KBehaviourOptions::defaults()
{
    cbNewWin->setChecked(false);

    homeURL->setText("~");

    rbOPLocal->setChecked(true);

    cbListProgress->setChecked( false );
}

void KBehaviourOptions::save()
{
    g_pConfig->setGroup( groupname );

    g_pConfig->writeEntry( "AlwaysNewWin", cbNewWin->isChecked() );
    g_pConfig->writeEntry( "HomeURL", homeURL->text() );

    QString val = QString::fromLatin1("Web only");
    if (rbOPWeb->isChecked())
        val = QString::fromLatin1("Local only");
    else if (rbOPNever->isChecked())
        val = QString::fromLatin1("Always");
    else if (rbOPAlways->isChecked())
        val = QString::fromLatin1("Never");
    kfmclientConfig->writeEntry(QString::fromLatin1("StartNewKonqueror"), val);

    kfmclientConfig->sync();

    g_pConfig->sync();

    // UIServer setting
    KConfig config("uiserverrc");
    config.setGroup( "UIServer" );
    config.writeEntry( "ShowList", cbListProgress->isChecked() );
    config.sync();
    // Tell the running server
    if ( kapp->dcopClient()->isApplicationRegistered( "kio_uiserver" ) )
    {
      UIServer_stub uiserver( "kio_uiserver", "UIServer" );
      uiserver.setListMode( cbListProgress->isChecked() );
    }
}

void KBehaviourOptions::updateWinPixmap(bool b)
{
  if (b)
    winPixmap->setPixmap(QPixmap(locate("data",
                                        "kcontrol/pics/overlapping.png")));
  else
    winPixmap->setPixmap(QPixmap(locate("data",
                                        "kcontrol/pics/onlyone.png")));
}

void KBehaviourOptions::changed()
{
  emit KCModule::changed(true);
}

#include "behaviour.moc"
