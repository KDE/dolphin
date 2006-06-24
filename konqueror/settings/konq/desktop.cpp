// -*- c-basic-offset: 2 -*-
/**
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
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

#include <QLabel>
#include <q3groupbox.h>
#include <QLayout>

#include <QCheckBox>
#include <QSlider>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <kapplication.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdialog.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kconfig.h>

#include <netwm.h>

#include "desktop.h"
#include "desktop.moc"
#ifdef Q_WS_X11
#include <QX11Info>
#endif

extern "C"
{
  KDE_EXPORT KCModule *create_virtualdesktops(QWidget *parent, const char * /*name*/)
  {
    KInstance *konq = new KInstance("kcmkonq");
    return new KDesktopConfig(konq, parent);
  }
}

// I'm using lineedits by intention as it makes sence to be able
// to see all desktop names at the same time. It also makes sense to
// be able to TAB through those line edits fast. So don't send me mails
// asking why I did not implement a more intelligent/smaller GUI.

KDesktopConfig::KDesktopConfig(KInstance *inst, QWidget *parent)
  : KCModule(inst, parent)
{

  setQuickHelp( i18n("<h1>Multiple Desktops</h1>In this module, you can configure how many virtual desktops you want and how these should be labeled."));

  Q_ASSERT(maxDesktops % 2 == 0);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(KDialog::spacingHint());

  // number group
  Q3GroupBox *number_group = new Q3GroupBox(this);

  QHBoxLayout *lay = new QHBoxLayout(number_group);
  lay->setMargin(KDialog::marginHint());
  lay->setSpacing(KDialog::spacingHint());

  QLabel *label = new QLabel(i18n("N&umber of desktops: "), number_group);
  _numInput = new KIntNumInput(4, number_group);
  _numInput->setRange(1, maxDesktops, 1, true);
  connect(_numInput, SIGNAL(valueChanged(int)), SLOT(slotValueChanged(int)));
  connect(_numInput, SIGNAL(valueChanged(int)), SLOT( changed() ));
  label->setBuddy( _numInput );
  QString wtstr = i18n( "Here you can set how many virtual desktops you want on your KDE desktop. Move the slider to change the value." );
  label->setWhatsThis( wtstr );
  _numInput->setWhatsThis( wtstr );

  lay->addWidget(label);
  lay->addWidget(_numInput);
  lay->setStretchFactor( _numInput, 2 );

  layout->addWidget(number_group);

  // name group
  Q3GroupBox *name_group = new Q3GroupBox(i18n("Desktop &Names"), this);

  name_group->setColumnLayout(4, Qt::Horizontal);

  for(int i = 0; i < (maxDesktops/2); i++)
    {
      _nameLabel[i] = new QLabel(i18n("Desktop %1:", i+1), name_group);
      _nameInput[i] = new KLineEdit(name_group);
      _nameLabel[i+(maxDesktops/2)] = new QLabel(i18n("Desktop %1:", i+(maxDesktops/2)+1), name_group);
      _nameInput[i+(maxDesktops/2)] = new KLineEdit(name_group);
      _nameLabel[i]->setWhatsThis( i18n( "Here you can enter the name for desktop %1", i+1 ) );
      _nameInput[i]->setWhatsThis( i18n( "Here you can enter the name for desktop %1", i+1 ) );
      _nameLabel[i+(maxDesktops/2)]->setWhatsThis( i18n( "Here you can enter the name for desktop %1", i+(maxDesktops/2)+1 ) );
      _nameInput[i+(maxDesktops/2)]->setWhatsThis( i18n( "Here you can enter the name for desktop %1", i+(maxDesktops/2)+1 ) );

      connect(_nameInput[i], SIGNAL(textChanged(const QString&)),
           SLOT( changed() ));
      connect(_nameInput[i+(maxDesktops/2)], SIGNAL(textChanged(const QString&)),
           SLOT( changed() ));
    }

  for(int i = 1; i < maxDesktops; i++)
      setTabOrder( _nameInput[i-1], _nameInput[i] );

  layout->addWidget(name_group);

  _wheelOption = new QCheckBox(i18n("Mouse wheel over desktop background switches desktop"), this);
  connect(_wheelOption,SIGNAL(toggled(bool)), SLOT( changed() ));

  layout->addWidget(_wheelOption);
  layout->addStretch(1);

  // Begin check for immutable
#ifdef Q_WS_X11
  int kwin_screen_number = DefaultScreen(QX11Info::display());
#else
  int kwin_screen_number = 0;
#endif

  KConfig config( "kwinrc" );

  QByteArray groupname;
  if (kwin_screen_number == 0)
     groupname = "Desktops";
  else
     groupname = "Desktops-screen-" + QByteArray::number ( kwin_screen_number );

  if (config.groupIsImmutable(groupname))
  {
     name_group->setEnabled(false);
     number_group->setEnabled(false);
  }
  else
  {
     KConfigGroup cfgGroup(&config, groupname);
     if (cfgGroup.entryIsImmutable("Number"))
     {
        number_group->setEnabled(false);
     }
  }
  // End check for immutable

  load();
}

void KDesktopConfig::load()
{
#ifdef Q_WS_X11
  // get number of desktops
  NETRootInfo info( QX11Info::display(), NET::NumberOfDesktops | NET::DesktopNames );
  int n = info.numberOfDesktops();

  _numInput->setValue(n);

  for(int i = 1; i <= maxDesktops; i++)
  {
    QString name = QString::fromUtf8(info.desktopName(i));
    _nameInput[i-1]->setText(name);
  }

  for(int i = 1; i <= maxDesktops; i++)
    _nameInput[i-1]->setEnabled(i <= n);
  emit changed(false);

  KConfig *desktopConfig = new KConfig("kdesktoprc", false, false);
  desktopConfig->setGroup("Mouse Buttons");
  _wheelOption->setChecked(desktopConfig->readEntry("WheelSwitchesWorkspace", QVariant(false)).toBool());

  _wheelOptionImmutable = desktopConfig->entryIsImmutable("WheelSwitchesWorkspace");

  if (_wheelOptionImmutable || n<2)
     _wheelOption->setEnabled(false);

  delete desktopConfig;
#else
  _numInput->setValue(1);
#endif
}

void KDesktopConfig::save()
{
#ifdef Q_WS_X11
  NETRootInfo info( QX11Info::display(), NET::NumberOfDesktops | NET::DesktopNames );
  // set desktop names
  for(int i = 1; i <= maxDesktops; i++)
  {
    info.setDesktopName(i, (_nameInput[i-1]->text()).toUtf8());
    info.activate();
  }
  // set number of desktops
  info.setNumberOfDesktops(_numInput->value());
  info.activate();

  XSync(QX11Info::display(), false);

  KConfig *desktopConfig = new KConfig("kdesktoprc");
  desktopConfig->setGroup("Mouse Buttons");
  desktopConfig->writeEntry("WheelSwitchesWorkspace", _wheelOption->isChecked());
  delete desktopConfig;

  // Tell kdesktop about the new config file
  int konq_screen_number = 0;
  if (QX11Info::display())
     konq_screen_number = DefaultScreen(QX11Info::display());

  QByteArray appname;
  if (konq_screen_number == 0)
      appname = "kdesktop";
  else
      appname = "kdesktop-screen-" + QByteArray::number( konq_screen_number );
#ifdef __GNUC__
#warning TODO Port to kdesktop DBus interface
#endif
  //kapp->dcopClient()->send( appname, "KDesktopIface", "configure()", data );

  emit changed(false);
#endif
}

void KDesktopConfig::defaults()
{
  int n = 4;
  _numInput->setValue(n);

  for(int i = 0; i < maxDesktops; i++)
    _nameInput[i]->setText(i18n("Desktop %1", i+1));

  for(int i = 0; i < maxDesktops; i++)
    _nameInput[i]->setEnabled(i < n);

  _wheelOption->setChecked(false);
  if (!_wheelOptionImmutable)
    _wheelOption->setEnabled(true);

  emit changed(false);
}

void KDesktopConfig::slotValueChanged(int n)
{
  for(int i = 0; i < maxDesktops; i++)
  {
    _nameInput[i]->setEnabled(i < n);
    if(i<n && _nameInput[i]->text().isEmpty())
      _nameInput[i]->setText(i18n("Desktop %1", i+1));
  }
  if (!_wheelOptionImmutable)
    _wheelOption->setEnabled(n>1);
  emit changed(true);
}

