/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

//-----------------------------------------------------------------------------
//
// Konqueror/KDesktop Fonts & Colors Options (for icon/tree view)
//
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
//
// KControl port & modifications
// (c) Torben Weis 1998
//
// End of the KControl port by David
// Port to KControl 2 by MHK
// konqy adaptations by David

#ifndef __KONQFONT_OPTIONS_H__
#define __KONQFONT_OPTIONS_H__

#include <qtabdialog.h>
#include <qcombobox.h>
#include <qstrlist.h>
#include <qcheckbox.h>
#include <kcolorbtn.h>
#include <qradiobutton.h>
#include <qfont.h>
#include <kconfig.h>
#include <kcmodule.h>
#include <kwm.h>


//-----------------------------------------------------------------------------

class KonqFontOptions : public KCModule
{
  Q_OBJECT
public:
  KonqFontOptions(KConfig *config, QString group, QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();

public slots:
  void slotFontSize( int );
  void slotStandardFont(const QString& n);

  void slotBgColorChanged( const QColor &col );
  void slotNormalTextColorChanged( const QColor &col );
  void slotHighlightedTextColorChanged( const QColor &col );

private slots:

  void changed();

private:
  void getFontList( QStrList &list, const char *pattern );
  void addFont( QStrList &list, const char *xfont );
  void updateGUI();

private:

  KConfig *g_pConfig;
  QString groupname;

  QRadioButton* m_pSmall;
  QRadioButton* m_pMedium;
  QRadioButton* m_pLarge;
  QComboBox* m_pStandard;

  int fSize;
  QString stdName;
  QFont m_stdFont;
  QStrList standardFonts;

  KColorButton* m_pBg;
  KColorButton* m_pNormalText;
  KColorButton* m_pHighlightedText;
  QColor bgColor;
  QColor normalTextColor;
  QColor highlightedTextColor;

  QCheckBox* m_pWordWrap;
  QCheckBox *cbUnderline;
};

#endif
