#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__


#include <kcmodule.h>
#include <kconfig.h>


class QCheckBox;
class QSlider;
class QLabel;


//-----------------------------------------------------------------------------


class KBehaviourOptions : public KCModule
{
  Q_OBJECT
public:
  KBehaviourOptions(KConfig *config, QString group, QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();


protected slots:
  
  virtual void slotClick();
  void changed();
 

private:

  KConfig *g_pConfig;
  QString groupname;

  QCheckBox *cbSingleClick;
  QCheckBox *cbAutoSelect;
  QLabel *lDelay;
  QSlider *slAutoSelect;
  QCheckBox *cbCursor;
  QCheckBox *cbUnderline;

};


#endif		// __BEHAVIOUR_H__

