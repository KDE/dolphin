// Behaviour options for konqueror


#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qwhatsthis.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdialog.h>
#include <konqdefaults.h>


#include "behaviour.h"

KBehaviourOptions::KBehaviourOptions(KConfig *config, QString group, bool showFileManagerOptions, QWidget *parent, const char *name )
    : KCModule(parent, name), g_pConfig(config), groupname(group), m_bFileManager(showFileManagerOptions)
{
    QLabel * label;
    int row = 0;

#define N_COLS 2
#define N_ROWS 9
    QGridLayout *lay = new QGridLayout(this,N_ROWS,N_COLS, // rows, cols
                                       KDialog::marginHint(),
				       KDialog::spacingHint());     // border, space
    lay->setRowStretch(N_ROWS-1,1);
    lay->setColStretch(N_COLS-1,1);

    // - only for konqueror, not for kdesktop --
    if (m_bFileManager)
    {
      row++;
      cbTreeFollow = new QCheckBox( i18n( "Tree follows navigation in other views" ), this );
      QWhatsThis::add( cbTreeFollow, i18n("This option only affects konqueror's behaviour if "
         "you have several views open, one of which is a tree view. Then, when you change directories "
         "in one view, the tree view will always highlight the current directory in the other view. This "
         "is a behaviour similar to what you may know from other file managers.") );
      lay->addMultiCellWidget( cbTreeFollow, row, row, 0, N_COLS, Qt::AlignLeft );
      connect( cbTreeFollow, SIGNAL( clicked() ), this, SLOT( changed() ) );

      row++;
      cbNewWin = new QCheckBox(i18n("&Open directories in separate windows"), this);
      QWhatsThis::add( cbNewWin, i18n("If this option is checked, konqueror will open a new window when "
         "you click on a directory, rather than changing to that directory in your old window."));
      lay->addMultiCellWidget(cbNewWin, row, row, 0, N_COLS-1, Qt::AlignLeft);
      connect(cbNewWin, SIGNAL(clicked()), this, SLOT(changed()));


      winPixmap = new QLabel(this);
      winPixmap->setPixmap(QPixmap(locate("data",
					"kcontrol/pics/onlyone.png")));
      lay->addWidget(winPixmap, row, N_COLS);
      connect(cbNewWin, SIGNAL(toggled(bool)), SLOT(updateWinPixmap(bool)));

      // ----
      row++;
      label = new QLabel(i18n("Home URL:"), this);
      lay->addWidget(label, row, 0);

      homeURL = new QLineEdit(this);
      lay->addWidget(homeURL, row, 1);
      connect(homeURL, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

      QString homestr = i18n("This is the URL (e.g. a directory or a web page) where "
         "konqueror will jump to when the \"Home\" button is pressed.");
      QWhatsThis::add( label, homestr );
      QWhatsThis::add( homeURL, homestr );

      // ----
      row++;
      QGroupBox *gbox = new QGroupBox(i18n("Use builtin viewer for"), this);
      lay->addMultiCellWidget(gbox,row,row,0,N_COLS,Qt::AlignLeft);
      QWhatsThis::add( gbox, i18n("These options determine whether certain content "
         "will be displayed in konqueror itself or if a separate application should be "
         "launched instead.") );

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

        cbTreeFollow->setChecked( g_pConfig->readBoolEntry( "TreeFollowsNavigation", DEFAULT_TREEFOLLOW ) );

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
        cbTreeFollow->setChecked( DEFAULT_TREEFOLLOW );

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
        g_pConfig->writeEntry( "TreeFollowsNavigation", cbTreeFollow->isChecked() );

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
