/*
   UserAgent Options
   (c) Kalle Dalheimer 1997

   Port to KControl
   (c) David Faure <faure@kde.org> 1998

   (c) Dirk Mueller <mueller@kde.org> 2000
   (c) Dawit Alemayehu <adawit@kde.org> 2000-2001
*/

#ifndef _USERAGENTDLG_H
#define _USERAGENTDLG_H "$Id$"

#include <kcmodule.h>
#include <kconfig.h>

class QLabel;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QStringList;
class QButtonGroup;

class KListView;
class FakeUASProvider;

class UserAgentOptions : public KCModule
{
  Q_OBJECT

public:
  UserAgentOptions ( QWidget * parent = 0L, const char * name = 0L) ;
  ~UserAgentOptions();

  virtual void load();
  virtual void save();
  virtual void defaults();
  QString quickHelp() const;

private slots:
  void addPressed();
  void deletePressed();
  void changePressed();
  void importPressed();
  void exportPressed();
  void updateButtons();
  void changeSendUAString();
  void changeDefaultUAModifiers( int );

private:
  enum {
    SHOW_OS = 0,
    SHOW_OS_VERSION,
    SHOW_PLATFORM,
    SHOW_MACHINE,
    SHOW_LANGUAGE
  };

  //Send User-agent String
  QCheckBox*      cb_sendUAString;

  // Default User-agent settings
  QLabel*         lb_default;
  QButtonGroup*   bg_default;
  QCheckBox*      cb_showPlatform;
  QCheckBox*      cb_showLanguage;
  QCheckBox*      cb_showMachine;
  QCheckBox*      cb_showOSV;
  QCheckBox*      cb_showOS;

  // Site specific settings
  KListView*      lv_siteUABindings;
  QGroupBox*      gb_siteSpecific;
  QPushButton*    pb_delete;
  QPushButton*    pb_change;
  QPushButton*    pb_import;
  QPushButton*    pb_export;
  QPushButton*    pb_add;

  // Useragent modifiers...
  QString m_ua_keys;
  // Fake user-agent modifiers...
  FakeUASProvider* m_provider;
  
  KConfig *m_config;
};

#endif
