//-----------------------------------------------------------------------------
//
// HTML Options
//
// (c) Martin R. Jones 1996
//
// Port to KControl
// (c) Torben Weis 1998

#ifndef __HTML_OPTIONS_H__
#define __HTML_OPTIONS_H__

#include <kcmodule.h>

class KColorButton;
class KConfig;
class KListView;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QListViewItem;
class QRadioButton;

//-----------------------------------------------------------------------------

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
  void slotStandardFont(const QString& n);
  void slotFixedFont(const QString& n);
  void slotCharset( const QString& n);
  void slotTextColorChanged( const QColor &col );
  void slotLinkColorChanged( const QColor &col );
  void slotVLinkColorChanged( const QColor &col );


private slots:

  void changed();


private:
  void getFontList( QStrList &list, const char *pattern );
  void addFont( QStrList &list, const char *xfont );
  void updateGUI();

private:

  KConfig *m_pConfig;
  QString m_groupname;

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

  KColorButton* m_pText;
  KColorButton* m_pLink;
  KColorButton* m_pVLink;
  QCheckBox *forceDefaultsbox;
  QColor bgColor;
  QColor textColor;
  QColor linkColor;
  QColor vLinkColor;

};


class KJavaScriptOptions : public KCModule
{
  Q_OBJECT
public:
  KJavaScriptOptions( KConfig* config, QString group, QWidget* parent = 0, const char* name = 0 );

  virtual void load();
  virtual void save();
  virtual void defaults();

private slots:
  void changed();
  void importPressed(); 
  void exportPressed();
  void addPressed();
  void changePressed();
  void deletePressed();
  void toggleJavaControls();

private:
  void changeJavaEnabled();
  void changeJavaScriptEnabled();
  void updateDomainList(const QStringList &domainConfig);

  KConfig *m_pConfig;
  QString m_groupname;

  QCheckBox* enableJavaGloballyCB;
  QCheckBox* enableJavaScriptGloballyCB;
  KListView* domainSpecificLV;
  QCheckBox* javaConsoleCB;
  QRadioButton* autoDetectRB;
  QRadioButton* userSpecifiedRB;
  QLineEdit* pathED;
  QLineEdit* addArgED;
  QMap<QListViewItem*, const char *> javaDomainPolicy;
  QMap<QListViewItem*, const char *> javaScriptDomainPolicy;
};


#endif		// __HTML_OPTIONS_H__

