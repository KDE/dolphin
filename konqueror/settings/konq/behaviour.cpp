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
      label = new QLabel(i18n("&Home URL:"), this);
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
      QString opstrIntro = i18n("With this option activated, only one instance of Konqueror "
                        "will exist in the memory of your computer at any moment "
                        "no matter how many ");
      QString opstrLocal = i18n("<b>local</b> ");
      QString opstrWeb   = i18n("<b>web</b> ");
      QString opstrMid   = i18n("browsing windows you open, thus reducing resource requirements."
                        "<p>Be aware that this also means that, if something goes wrong, "
                        "all your ");
      QString opstrClose = i18n("windows will be closed simultaneously");

      bgOneProcess = new QVButtonGroup(i18n("Minimize memory usage"), this);
      bgOneProcess->setExclusive( true );
      connect(bgOneProcess, SIGNAL(clicked(int)), this, SLOT(changed()));
      {
        rbOPNever = new QRadioButton(i18n("&Never"), bgOneProcess);
        QWhatsThis::add( rbOPNever, i18n("Disables the minimization of memory usage and allows you "
                                         "to make each browsing activity independent from the others"));

        rbOPLocal = new QRadioButton(i18n("For &local browsing only (recommended)"), bgOneProcess);
        QWhatsThis::add( rbOPLocal, opstrIntro + opstrLocal + opstrMid + opstrLocal + opstrClose);

        rbOPWeb = new QRadioButton(i18n("For &web browsing only"), bgOneProcess);
        QWhatsThis::add( rbOPWeb, opstrIntro + opstrWeb + opstrMid + opstrWeb + opstrClose);

        rbOPAlways = new QRadioButton(i18n("&Always (use with care)"), bgOneProcess);
        QWhatsThis::add( rbOPAlways, opstrIntro +  opstrMid + opstrClose);

        rbOPLocal->setChecked(true);
      }

      lay->addMultiCellWidget(bgOneProcess, row, row, 0, 3);

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
    }
}

void KBehaviourOptions::defaults()
{
    if (m_bFileManager)
    {
        cbNewWin->setChecked(false);

        homeURL->setText("~");

        rbOPLocal->setChecked(true);
    }
}

void KBehaviourOptions::save()
{
    g_pConfig->setGroup( groupname );

    if (m_bFileManager)
    {
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
