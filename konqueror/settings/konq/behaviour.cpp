// Behaviour options for konqueror


#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qslider.h>
#include <qwhatsthis.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <konqdefaults.h>
#include <kstddirs.h>

#include "behaviour.h"

KBehaviourOptions::KBehaviourOptions(KConfig *config, QString group, bool showFileManagerOptions, QWidget *parent, const char *name )
    : KCModule(parent, name), g_pConfig(config), groupname(group), m_bFileManager(showFileManagerOptions)
{
    QLabel * label;
    int row = 0;

    QGridLayout *lay = new QGridLayout(this,9,4, // rows, cols
                                       KDialog::marginHint(),
				       KDialog::spacingHint());     // border, space
    lay->setRowStretch(6,1);
    lay->setColStretch(1,1);
    lay->setColStretch(3,1);

    // - only for konqueror, not for kdesktop --
    if (m_bFileManager)
    {
      row++;
      winPixmap = new QLabel(this);
      winPixmap->setPixmap(QPixmap(locate("data",
					"kcontrol/pics/onlyone.png")));
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
      label = new QLabel(i18n("Home URL:"), this);
      lay->addWidget(label, row, 0);

      homeURL = new QLineEdit(this);
      lay->addMultiCellWidget(homeURL, row, row, 1, 3);
      connect(homeURL, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

      QString homestr = i18n("This is the URL (e.g. a directory or a web page) where "
         "konqueror will jump to when the \"Home\" button is pressed. Usually a 'tilde' (~).");
      QWhatsThis::add( label, homestr );
      QWhatsThis::add( homeURL, homestr );

      // ----
      row++;
      QGroupBox *gbox = new QGroupBox(i18n("Use builtin viewer for"), this);
      lay->addMultiCellWidget(gbox,row,row,0,2,Qt::AlignLeft);
      QWhatsThis::add( gbox, i18n("These options determine whether certain types of content "
         "should be displayed embedded in Konqueror itself. If unchecked, Konqueror will launch the "
         "application associated with the file type. Please note that these"
         " settings only take effect on files for which you have not specified"
         " a special behavior in the 'File Associations' control module.") );

      QGridLayout *grid = new QGridLayout(gbox, 4, 0,
                                          KDialog::marginHint(),
                                          KDialog::spacingHint());
      grid->addRowSpacing(0,10);
      grid->setRowStretch(0,0);

      cbEmbedText = new QCheckBox( i18n("Text files"), gbox );
      grid->addWidget( cbEmbedText, 1, 0 );
      connect(cbEmbedText, SIGNAL(clicked()), this, SLOT(changed()));

      cbEmbedImage = new QCheckBox( i18n("Image files"), gbox );
      grid->addWidget( cbEmbedImage, 2, 0 );
      connect(cbEmbedImage, SIGNAL(clicked()), this, SLOT(changed()));

      cbEmbedOther = new QCheckBox( i18n("Other (DVI,PS...)"), gbox );
      grid->addWidget( cbEmbedOther, 3, 0 );
      connect(cbEmbedOther, SIGNAL(clicked()), this, SLOT(changed()));

      gbox->setMinimumWidth( cbEmbedOther->width()+50 ); // workaround for groupbox title truncated
    }

    load();
}


void KBehaviourOptions::load()
{
    g_pConfig->setGroup( groupname );
    if (m_bFileManager)
    {
        cbNewWin->setChecked( g_pConfig->readBoolEntry("AlwaysNewWin", false) );
        updateWinPixmap(cbNewWin->isChecked());

        homeURL->setText(g_pConfig->readEntry("HomeURL", "~"));
        bool embedText = g_pConfig->readBoolEntry("EmbedText", true);
        bool embedImage = g_pConfig->readBoolEntry("EmbedImage", true);
        bool embedOther = g_pConfig->readBoolEntry("EmbedOther", true);

        cbEmbedText->setChecked( embedText );
        cbEmbedImage->setChecked( embedImage );
        cbEmbedOther->setChecked( embedOther );
    }
}

void KBehaviourOptions::defaults()
{
    if (m_bFileManager)
    {
        cbNewWin->setChecked(false);

        homeURL->setText("~");

        cbEmbedText->setChecked( true );
        cbEmbedImage->setChecked( true );
        cbEmbedOther->setChecked( true );
    }
}

void KBehaviourOptions::save()
{
    g_pConfig->setGroup( groupname );

    if (m_bFileManager)
    {
        g_pConfig->writeEntry( "AlwaysNewWin", cbNewWin->isChecked() );
        g_pConfig->writeEntry( "HomeURL", homeURL->text() );
        g_pConfig->writeEntry( "EmbedText", cbEmbedText->isChecked() );
        g_pConfig->writeEntry( "EmbedImage", cbEmbedImage->isChecked() );
        g_pConfig->writeEntry( "EmbedOther", cbEmbedOther->isChecked() );
    }

    g_pConfig->sync();
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
