//-----------------------------------------------------------------------------
//
// KDE Help Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998

#ifndef __HTML_OPTIONS_H__
#define __HTML_OPTIONS_H__

#include <qtabdialog.h>
#include <qcombobox.h>
#include <qstrlist.h>
#include <qcheckbox.h>
#include <kconfig.h>
#include <kcolorbtn.h>
#include <qradiobutton.h>
#include <kcharsets.h>

#include <kcontrol.h>
#include <kwm.h>

extern KConfig *g_pConfig;

//-----------------------------------------------------------------------------

class KFontOptions : public KConfigWidget
{
  Q_OBJECT
public:
  KFontOptions( QWidget *parent, const char *name, const char *groupName );

  virtual void loadSettings();
  virtual void saveSettings();
  virtual void applySettings();
  virtual void defaultSettings();

public slots:
  void slotFontSize( int );
  void slotStandardFont(const QString& n);
  void slotFixedFont(const QString& n);
  void slotCharset( const QString& n);

private:
  void getFontList( QStrList &list, const char *pattern );
  void addFont( QStrList &list, const char *xfont );
  void updateGUI();

private:
  QRadioButton* m_pSmall;
  QRadioButton* m_pMedium;
  QRadioButton* m_pLarge;
  QComboBox* m_pFixed;
  QComboBox* m_pStandard;
  QComboBox* m_pCharset;

  int fSize;
  QString stdName;
  QString fixedName;
  QString charsetName;
  QStrList standardFonts;
  QStrList fixedFonts;
  QStringList charsets;

  QCString m_sGroup;
};

//-----------------------------------------------------------------------------

class KColorOptions : public KConfigWidget
{
  Q_OBJECT
public:
  KColorOptions( QWidget *parent, const char *name, const char *groupName );

  virtual void loadSettings();
  virtual void saveSettings();
  virtual void applySettings();
  virtual void defaultSettings();

protected slots:
  void slotBgColorChanged( const QColor &col );
  void slotTextColorChanged( const QColor &col );
  void slotLinkColorChanged( const QColor &col );
  void slotVLinkColorChanged( const QColor &col );

private:
  KColorButton* m_pBg;
  KColorButton* m_pText;
  KColorButton* m_pLink;
  KColorButton* m_pVLink;
  QCheckBox *forceDefaultsbox;
  QColor bgColor;
  QColor textColor;
  QColor linkColor;
  QColor vLinkColor;

  QCString m_sGroup;
};

class KHtmlOptions : public KConfigWidget
{
  Q_OBJECT
public:
  KHtmlOptions( QWidget *parent, const char *name);

  virtual void loadSettings();
  virtual void saveSettings();
  virtual void applySettings();
  virtual void defaultSettings();

private:
    QLineEdit *le_JavaPath;
    QCheckBox *cb_enableJava;
    QCheckBox *cb_enableJavaScript;
    QCheckBox *m_pAutoLoadImagesCheckBox;

};

#endif		// __HTML_OPTIONS_H__

