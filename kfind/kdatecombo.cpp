/***********************************************************************
 *
 *  kdatecombo.cpp
 *
 ***********************************************************************/

#include <QTimer>
//Added by qt3to4:
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>

#include <kglobal.h>
#include <klocale.h>
#include <kdatepicker.h>
#include <kdatetable.h>
#include <kdebug.h>

#include "kdatecombo.h"

#include "kdatecombo.moc"

KDateCombo::KDateCombo(QWidget *parent) : QComboBox(parent)
{
  setEditable( false );

  QDate date = QDate::currentDate();
  initObject(date);
}

KDateCombo::KDateCombo(const QDate & date, QWidget *parent) : QComboBox(parent)
{
  setEditable( false );

  initObject(date);
}

void KDateCombo::initObject(const QDate & date)
{
  setValidator(0);
  popupFrame = new KPopupFrame(this);
  popupFrame->installEventFilter(this);
  datePicker = new KDatePicker(date, popupFrame);
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
    addItem(date2String(newDate));
    return true;
  }
  return false;
}

void KDateCombo::dateEnteredEvent(QDate newDate)
{
  if (!newDate.isValid())
     newDate = datePicker->date();
  popupFrame->hide();
  setDate(newDate);
}

void KDateCombo::mousePressEvent (QMouseEvent * e)
{
  if (e->button() & Qt::LeftButton)
  {
    if  (rect().contains( e->pos()))
    {
      QDate tempDate;
      getDate(& tempDate);
      datePicker->setDate(tempDate);
      popupFrame->popup(mapToGlobal(QPoint(0, height())));
      //datePicker->setFocus();
    }
  }
}

bool KDateCombo::eventFilter (QObject*, QEvent* e)
{
  if ( e->type() == QEvent::MouseButtonPress )
  {
      QMouseEvent *me = (QMouseEvent *)e;
      QPoint p = mapFromGlobal( me->globalPos() );
      if (rect().contains( p ) )
      {
        QTimer::singleShot(10, this, SLOT(dateEnteredEvent()));
        return true;
      }
  }
  else if ( e->type() == QEvent::KeyRelease )
  {
      QKeyEvent *k = (QKeyEvent *)e;
      //Press return == pick selected date and close the combo
      if((k->key()==Qt::Key_Return)||(k->key()==Qt::Key_Enter))
      {
        dateEnteredEvent(datePicker->date());
        return true;
      }
      else if (k->key()==Qt::Key_Escape)
      {
        popupFrame->hide();
        return true;
      }
      else
        return false;
  }

  return false;
}
