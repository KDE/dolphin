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
#include <kprotocolmanager.h>

class QLabel;
class QCheckBox;
class QGroupBox;
class QPushButton;
class QStringList;
class QButtonGroup;

class KListView;

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
  QCheckBox*      cb_showOS;
  QCheckBox*      cb_showOSV;
  QCheckBox*      cb_showPlatform;
  QCheckBox*      cb_showMachine;
  QCheckBox*      cb_showLanguage;

  // Site specific settings
  QPushButton*    pb_add;
  QPushButton*    pb_delete;
  QPushButton*    pb_change;
  QPushButton*    pb_import;
  QPushButton*    pb_export;
  QGroupBox*      gb_siteSpecific;
  KListView*      lv_siteUABindings;

  // Useragent modifiers
  KProtocolManager::UAMODIFIERS m_iMods;
};

#endif
