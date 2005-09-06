/**
 * socks.h
 *
 * Copyright (c) 2001 George Staikos <staikos@kde.org>
 * Copyright (c) 2001 Daniel Molkentin <molkentin@kde.org> (designer port)
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
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _SOCKS_H
#define _SOCKS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kcmodule.h>

#include "socksbase.h"


class QVButtonGroup;

class KSocksConfig : public KCModule
{
  Q_OBJECT
public:
  KSocksConfig(QWidget *parent);
  virtual ~KSocksConfig();

  SocksBase *base;

  void load();
  void save();
  void defaults();

  int buttons();
  QString quickHelp() const;

public slots:
  void configChanged();

private slots:
  void enableChanged();
  void methodChanged(int id);
  void testClicked();
  void chooseCustomLib(KURLRequester *url);
  void customPathChanged(const QString&);
  void addLibrary();
  void libTextChanged(const QString& lib);
  void addThisLibrary(const QString& lib);
  void removeLibrary();
  void libSelection();

private:

  bool _socksEnabled;
  int _useWhat;
};

#endif
