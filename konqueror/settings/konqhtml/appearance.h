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

#include <kcmodule.h>
#include <qmap.h>

class QSpinBox;
class KFontCombo;

class KAppearanceOptions : public KCModule
{
  Q_OBJECT
public:
  KAppearanceOptions(KConfig *config, QString group, QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();

public slots:
  void slotFontSize( int );
  void slotMinimumFontSize( int );
  void slotStandardFont(const QString& n);
  void slotFixedFont(const QString& n);
  void slotSerifFont( const QString& n );
  void slotSansSerifFont( const QString& n );
  void slotCursiveFont( const QString& n );
  void slotFantasyFont( const QString& n );
  void slotEncoding( const QString& n);
  void slotCharset( const QString &n );
  void slotFontSizeAdjust( int value );
  void slotEnforceDefault( bool );
    
private slots:
  void changed();

private:
  void updateGUI();

private:

  KConfig *m_pConfig;
  QString m_groupname;
  QStringList m_families;

  KIntNumInput* m_minSize;
  KIntNumInput* m_MedSize;
  KIntNumInput* m_pageDPI;
  KFontCombo* m_pFonts[6];
  QComboBox* m_pEncoding;
  QComboBox* m_pChset;
  QCheckBox* m_pEnforceCharset;

  QSpinBox *m_pFontSizeAdjust;

  bool enforceCharset;
  int fSize;
  int fMinSize;
  QMap<QString, QStringList> fontsForCharset;
  QStringList encodings;
  QStringList chSets;
  QString charset;
  QStringList fonts;
  QStringList defaultFonts;
  QString encodingName;
};

#endif // __APPEARANCE_H__
