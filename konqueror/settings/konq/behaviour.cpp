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
#include <konq_defaults.h>
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
      label = new QLabel(i18n("Home URL:"), this);
      lay->addWidget(label, row, 0);

      homeURL = new QLineEdit(this);
      lay->addMultiCellWidget(homeURL, row, row, 1, 3);
      connect(homeURL, SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

      QString homestr = i18n("This is the URL (e.g. a directory or a web page) where "
         "konqueror will jump to when the \"Home\" button is pressed. Usually a 'tilde' (~).");
      QWhatsThis::add( label, homestr );
      QWhatsThis::add( homeURL, homestr );

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
    }
}

void KBehaviourOptions::defaults()
{
    if (m_bFileManager)
    {
        cbNewWin->setChecked(false);

        homeURL->setText("~");
    }
}

void KBehaviourOptions::save()
{
    g_pConfig->setGroup( groupname );

    if (m_bFileManager)
    {
        g_pConfig->writeEntry( "AlwaysNewWin", cbNewWin->isChecked() );
        g_pConfig->writeEntry( "HomeURL", homeURL->text() );
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
