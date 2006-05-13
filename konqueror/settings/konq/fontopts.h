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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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

#include <QStringList>
#include <QSpinBox>

#include <kcmodule.h>

class QCheckBox;
class QRadioButton;

class KColorButton;
class KConfig;
class KFontCombo;


//-----------------------------------------------------------------------------

class KonqFontOptions : public KCModule
{
  Q_OBJECT
public:
  KonqFontOptions( KConfig *config, QString group, bool desktop, KInstance *inst, QWidget *parent);
  QString quickHelp() const;

  virtual void load();
  virtual void save();
  virtual void defaults();

public Q_SLOTS:
  void slotFontSize(int i);
  void slotStandardFont(const QString& n);
  void slotTextBackgroundClicked();

  void slotNormalTextColorChanged( const QColor &col );
  //void slotHighlightedTextColorChanged( const QColor &col );
  void slotTextBackgroundColorChanged( const QColor &col );

private slots:
  void slotPNbLinesChanged(int value);
  void slotPNbWidthChanged(int value);

private:
  void updateGUI();

private:

  KConfig *g_pConfig;
  QString groupname;
  bool m_bDesktop;

  /*
  QRadioButton* m_pSmall;
  QRadioButton* m_pMedium;
  QRadioButton* m_pLarge;
  */
  KFontCombo* m_pStandard;
  QSpinBox* m_pSize;

  int m_fSize;
  QString m_stdName;

  KColorButton* m_pBg;
  KColorButton* m_pNormalText;
  //KColorButton* m_pHighlightedText;
  QCheckBox* m_cbTextBackground;
  KColorButton* m_pTextBackground;
  QColor normalTextColor;
  //QColor highlightedTextColor;
  QColor textBackgroundColor;

  QSpinBox* m_pNbLines;
  QSpinBox* m_pNbWidth;
  QCheckBox* cbUnderline;
  QCheckBox* m_pSizeInBytes;
};

#endif
