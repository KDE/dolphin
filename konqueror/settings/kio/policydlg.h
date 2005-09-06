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
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _POLICYDLG_H
#define _POLICYDLG_H

#include <qstring.h>

#include <kdialogbase.h>


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

    if (advice.find (QString::fromLatin1("accept"), 0, false) == 0)
      return KCookieAdvice::Accept;
    else if (advice.find (QString::fromLatin1("reject"), 0, false) == 0)
      return KCookieAdvice::Reject;
    else if (advice.find (QString::fromLatin1("ask"), 0, false) == 0)
      return KCookieAdvice::Ask;

    return KCookieAdvice::Dunno;
  }
};

class PolicyDlg : public KDialogBase
{
  Q_OBJECT

public:
  PolicyDlg (const QString& caption, QWidget *parent = 0,
                    const char *name = 0);
  ~PolicyDlg (){};

  int advice() const;
  QString domain() const;

  void setEnableHostEdit( bool, const QString& host = QString::null );
  void setPolicy (int policy);

protected slots:
  void slotTextChanged( const QString& );

private:
  PolicyDlgUI* m_dlgUI;
};
#endif
