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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qlabel.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qslider.h>

#include <kconfig.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdialog.h>
#include <klineedit.h>
#include <netwm.h>

#include "desktop.h"
#include "desktop.moc"

extern "C"
{
  KCModule *create_virtualdesktops(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmkonq");
    return new KDesktopConfig(parent, name);
  };
}

// I'm using 16 line inputs by intention as it makes sence to be able
// to see all desktop names at the same time. It also makes sence to
// be able to TAB through those line edits fast. So don't send me mails
// asking why I did not implement a more intelligent/smaller GUI.

KDesktopConfig::KDesktopConfig(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  QVBoxLayout *layout = new QVBoxLayout(this,
                    KDialog::marginHint(),
                    KDialog::spacingHint());

  // number group
  QGroupBox *number_group = new QGroupBox("", this);

  QHBoxLayout *lay = new QHBoxLayout(number_group,
                     KDialog::marginHint(),
                     KDialog::spacingHint());

  QLabel *label = new QLabel(i18n("&Number of Desktops: "), number_group);
  _numLabel = new QLabel("4", number_group);
  _numSlider = new QSlider(1, 16, 1, 4, Horizontal, number_group);
  connect(_numSlider, SIGNAL(valueChanged(int)), SLOT(slotValueChanged(int)));
  label->setBuddy( _numSlider );
  QString wtstr = i18n( "Here you can set how many virtual desktops you want on your KDE desktop. Move the slider to change the value." );
  QWhatsThis::add( label, wtstr );
  QWhatsThis::add( _numLabel, wtstr );
  QWhatsThis::add( _numSlider, wtstr );

  lay->addWidget(label);
  lay->addWidget(_numLabel);
  lay->addWidget(_numSlider);

  lay->setStretchFactor(_numLabel, 2);
  lay->setStretchFactor(_numSlider, 3);

  layout->addWidget(number_group);

  // name group
  QGroupBox *name_group = new QGroupBox("", this);

  QGridLayout *grid = new QGridLayout(name_group, 8, 4,
                      KDialog::marginHint(),
                      KDialog::spacingHint());

  for(int i = 0; i < 8; i++)
    {
      _nameLabel[i] = new QLabel(i18n("Desktop%1:").arg(i+1), name_group);
      _nameLabel[i+8] = new QLabel(i18n("Desktop%1:").arg(i+8+1), name_group);
      _nameInput[i] = new KLineEdit(name_group);
      _nameInput[i+8] = new KLineEdit(name_group);
      QWhatsThis::add( _nameLabel[i], i18n( "Here you can enter the name for desktop %1" ).arg( i+1 ) );
      QWhatsThis::add( _nameInput[i], i18n( "Here you can enter the name for desktop %1" ).arg( i+1 ) );
      QWhatsThis::add( _nameLabel[i+8], i18n( "Here you can enter the name for desktop %1" ).arg( i+8+1 ) );
      QWhatsThis::add( _nameInput[i+8], i18n( "Here you can enter the name for desktop %1" ).arg( i+8+1 ) );

      connect(_nameInput[i], SIGNAL(textChanged(const QString&)),
          SLOT(slotTextChanged(const QString&)));
      connect(_nameInput[i+8], SIGNAL(textChanged(const QString&)),
          SLOT(slotTextChanged(const QString&)));

      grid->addWidget(_nameLabel[i], i, 0);
      grid->addWidget(_nameInput[i], i, 1);
      grid->addWidget(_nameLabel[i+8], i, 2);
      grid->addWidget(_nameInput[i+8], i, 3);
    }

  grid->setColStretch(1, 2);
  grid->setColStretch(3, 2);

  layout->addWidget(name_group);
  layout->setStretchFactor(name_group, 2);
  layout->addStretch(1);

  load();
}

void KDesktopConfig::load()
{
  // get number of desktops
  NETRootInfo info( qt_xdisplay(), NET::NumberOfDesktops | NET::DesktopNames );
  int n = info.numberOfDesktops();

  _numSlider->setValue(n);

  for(int i = 1; i <= 16; i++)
    _nameInput[i-1]->setText(QString::fromUtf8(info.desktopName(i)));

  for(int i = 1; i <= 16; i++)
    _nameInput[i-1]->setEnabled(i <= n);
  emit changed(false);
}

void KDesktopConfig::save()
{
  // set number of desktops
  NETRootInfo info( qt_xdisplay(), NET::NumberOfDesktops | NET::DesktopNames );
  info.setNumberOfDesktops(_numSlider->value());
  XSync(qt_xdisplay(), FALSE);
  info.activate();

  // set desktop names
  for(int i = 1; i <= 16; i++)
    info.setDesktopName(i, (_nameInput[i-1]->text()).utf8());

  emit changed(false);
}

void KDesktopConfig::defaults()
{
  int n = 4;
  _numSlider->setValue(n);
  _numLabel->setText(QString("%1").arg(n));

  for(int i = 0; i < 16; i++)
    _nameInput[i]->setText(i18n("Desktop %1").arg(i+1));

  for(int i = 0; i < 16; i++)
    _nameInput[i]->setEnabled(i < n);
  emit changed(false);
}

QString KDesktopConfig::quickHelp() const
{
  return i18n("<h1>Desktop Number & Names</h1>In this module, you can configure how many virtual desktops you want and how these should be labelled.");
}

void KDesktopConfig::slotValueChanged(int n)
{
  for(int i = 0; i < 16; i++)
  {
    _nameInput[i]->setEnabled(i < n);
    if(i<n && _nameInput[i]->text().isEmpty())
      _nameInput[i]->setText(i18n("Desktop %1").arg(i+1));
  }
  _numLabel->setText(QString("%1").arg(n));
  emit changed(true);
}

void KDesktopConfig::slotTextChanged(const QString&)
{
  emit changed(true);
}
