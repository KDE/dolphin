/**
 * Copyright (c) 2000- Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _POLICYDLG_H
#define _POLICYDLG_H

#include <QString>

#include <kdialog.h>


class QPushButton;
class PolicyDlgUI;

class KCookieAdvice
{
public:
  enum Value {Dunno=0, Accept, Reject, Ask};

  static const char * adviceToStr (const int& advice)
  {
    switch (advice)
    {
      case KCookieAdvice::Accept:
        return I18N_NOOP("Accept");
      case KCookieAdvice::Reject:
        return I18N_NOOP("Reject");
      case KCookieAdvice::Ask:
        return I18N_NOOP("Ask");
      default:
        return I18N_NOOP("Dunno");
    }
  }

  static KCookieAdvice::Value strToAdvice (const QString& advice)
  {
    if (advice.isEmpty())
      return KCookieAdvice::Dunno;

    if (advice.indexOf (QLatin1String("accept"), 0, Qt::CaseInsensitive) == 0)
      return KCookieAdvice::Accept;
    else if (advice.indexOf (QLatin1String("reject"), 0, Qt::CaseInsensitive) == 0)
      return KCookieAdvice::Reject;
    else if (advice.indexOf (QLatin1String("ask"), 0, Qt::CaseInsensitive) == 0)
      return KCookieAdvice::Ask;

    return KCookieAdvice::Dunno;
  }
};

class PolicyDlg : public KDialog
{
  Q_OBJECT

public:
  PolicyDlg (const QString& caption, QWidget *parent = 0,
                    const char *name = 0);
  ~PolicyDlg (){};

  int advice() const;
  QString domain() const;

  void setEnableHostEdit( bool, const QString& host = QString() );
  void setPolicy (int policy);

protected Q_SLOTS:
  void slotTextChanged( const QString& );

private:
  PolicyDlgUI* m_dlgUI;
};
#endif
