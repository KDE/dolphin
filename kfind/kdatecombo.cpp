/***********************************************************************
 *
 *  kdatecombo.cpp
 *
 ***********************************************************************/

#include <kglobal.h>
#include <klocale.h>
#include <kdatepicker.h>
#include <kdatetbl.h>
#include <kdebug.h>

#include "kdatecombo.h"

#include "kdatecombo.moc"

KDateCombo::KDateCombo(QWidget *parent, const char *name ) : QComboBox(FALSE, parent,name)
{
  QDate date = QDate::currentDate();
  initObject(date, parent, name);
}

KDateCombo::KDateCombo(const QDate & date, QWidget *parent, const char *name) : QComboBox(FALSE, parent,name)
{
  initObject(date, parent, name);
}

void KDateCombo::initObject(const QDate & date, QWidget *parent, const char *name)
{
  clearValidator();
  popupFrame = new KPopupFrame(this, "popupFrame");
  datePicker = new KDatePicker(popupFrame, date, "datePicker");
  datePicker->setMinimumSize(datePicker->sizeHint());
  datePicker->installEventFilter(this);
  popupFrame->setMainWidget(datePicker);
  setDate(date);

  connect(datePicker, SIGNAL(dateSelected(QDate)), this, SLOT(dateEnteredEvent(QDate)));
}

KDateCombo::~KDateCombo()
{
  delete datePicker;
  delete popupFrame;
}

QString KDateCombo::date2String(const QDate & date)
{
  return(KGlobal::locale()->formatDate(date, true));
}

QDate & KDateCombo::string2Date(const QString & str, QDate *qd)
{
  return *qd = KGlobal::locale()->readDate(str);
}

QDate & KDateCombo::getDate(QDate *currentDate)
{
  return string2Date(currentText(), currentDate);
}

bool KDateCombo::setDate(const QDate & newDate)
{
  if (newDate.isValid())
  {
    if (count())
      clear();
    insertItem(date2String(newDate));
    return TRUE;
  }
  return FALSE;
}

void KDateCombo::dateEnteredEvent(QDate newDate)
{
  popupFrame->hide();
  setDate(newDate);
}

void KDateCombo::mousePressEvent (QMouseEvent * e)
{
  if (e->button() & QMouseEvent::LeftButton)
    if  (rect().contains( e->pos()))
    {
      QDate tempDate;
      getDate(& tempDate);
      datePicker->setDate(tempDate);
      popupFrame->popup(mapToGlobal(QPoint(0, height())));
      //datePicker->setFocus();
    }
}

bool KDateCombo::eventFilter (QObject* o, QEvent* e)
{
  if ( e->type() == QEvent::KeyRelease )
  {
      QKeyEvent *k = (QKeyEvent *)e;
      //Press return == pick selected date and close the combo
      if((k->key()==Qt::Key_Return)||(k->key()==Qt::Key_Enter))
      {
        dateEnteredEvent(datePicker->getDate());
        return true;
      }
      else
        return false;
  }
  else
    return false;
}
