#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__

#include <kcontrol.h>

class QCheckBox;
class QSlider;
class KConfig;
extern KConfig *g_pConfig;

//-----------------------------------------------------------------------------

class KBehaviourOptions : public KConfigWidget
{
  Q_OBJECT
public:
  KBehaviourOptions( QWidget *parent, const char *name );

  virtual void loadSettings();
  virtual void saveSettings();
  virtual void applySettings();
  virtual void defaultSettings();

protected slots:
  
  virtual void slotClick();

private:
  QCheckBox *cbSingleClick;
  QCheckBox *cbAutoSelect;
  QLabel *lDelay;
  QSlider *slAutoSelect;
  QCheckBox *cbCursor;
  QCheckBox *cbUnderline;
};

#endif		// __BEHAVIOUR_H__

