#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__


#include <kcmodule.h>


class QCheckBox;
class QLabel;
class QLineEdit;
class KConfig;

//-----------------------------------------------------------------------------


class KBehaviourOptions : public KCModule
{
  Q_OBJECT
public:
  /**
   * @param showFileManagerOptions : set to true for konqueror, false for kdesktop
   */
  KBehaviourOptions(KConfig *config, QString group, bool showFileManagerOptions, QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();


protected slots:

  void changed();
  void updateWinPixmap(bool);

private:

  KConfig *g_pConfig;
  QString groupname;
  bool m_bFileManager;

  QCheckBox *cbNewWin;

  QLabel *winPixmap;

  QLineEdit *homeURL;
};


#endif		// __BEHAVIOUR_H__

