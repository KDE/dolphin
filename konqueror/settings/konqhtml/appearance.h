// -*- c-basic-offset: 2 -*-
// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// Cleanup and modifications for KDE 2.1
// (c) Daniel Molkentin 2000

#ifndef __APPEARANCE_H__
#define __APPEARANCE_H__

#include <QWidget>
#include <QMap>

#include <kcmodule.h>
#include <kconfig.h>

class QSpinBox;
class QFontComboBox;

class KAppearanceOptions : public KCModule
{
  Q_OBJECT
public:
  KAppearanceOptions(QWidget *parent, const QStringList&);
  ~KAppearanceOptions();

  virtual void load();
  virtual void save();
  virtual void defaults();

public Q_SLOTS:
  void slotFontSize( int );
  void slotMinimumFontSize( int );
  void slotStandardFont(const QFont& n);
  void slotFixedFont(const QFont& n);
  void slotSerifFont( const QFont& n );
  void slotSansSerifFont( const QFont& n );
  void slotCursiveFont( const QFont& n );
  void slotFantasyFont( const QFont& n );
  void slotEncoding( const QString& n);
  void slotFontSizeAdjust( int value );

private:
  void updateGUI();

private:

  KSharedConfig::Ptr m_pConfig;
  QString m_groupname;

  KIntNumInput* m_minSize;
  KIntNumInput* m_MedSize;
  KIntNumInput* m_pageDPI;
  QFontComboBox* m_pFonts[6];
  QComboBox* m_pEncoding;
  QSpinBox *m_pFontSizeAdjust;

  int fSize;
  int fMinSize;
  QStringList encodings;
  QStringList fonts;
  QStringList defaultFonts;
  QString encodingName;
};

#endif // __APPEARANCE_H__
