/*
   cache.h - Proxy configuration dialog

   Copyright (C) 2001,02,03 Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef CACHE_H
#define CACHE_H

#include <kcmodule.h>
#include "ui_cache_ui.h"

class CacheDlgUI : public QWidget, public Ui::CacheDlgUI
{
    public:
        CacheDlgUI(QWidget *parent = 0)
        : QWidget(parent)
        {
            setupUi(this);
        }
};


class KCacheConfigDialog : public KCModule
{
  Q_OBJECT

public:
  KCacheConfigDialog(QWidget *parent, const QStringList &args);
  ~KCacheConfigDialog() {}

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

protected Q_SLOTS:
  void configChanged();
  void slotClearCache();

private:
  CacheDlgUI* m_dlg;
};

#endif // CACHE_H
