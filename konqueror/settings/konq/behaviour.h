#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__


#include <kcmodule.h>


class QCheckBox;
class QSlider;
class QLabel;


//-----------------------------------------------------------------------------


class KBehaviourOptions : public KCModule
{
  Q_OBJECT
public:
  KBehaviourOptions( QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();


protected slots:
  
  virtual void slotClick();
  void changed();
 

private:
  QCheckBox *cbSingleClick;
  QCheckBox *cbAutoSelect;
  QLabel *lDelay;
  QSlider *slAutoSelect;
  QCheckBox *cbCursor;
  QCheckBox *cbUnderline;

};


#endif		// __BEHAVIOUR_H__

