/***************************************************************************
 *   Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   *
 *   Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

#ifndef CHFNPROC_H
#define CHFNPROC_H

#include <QByteArray>
#include <kdesu/process.h>

class ChfnProcess : public PtyProcess
{
public:

  enum Errors { ChfnNotFound=1, PasswordError=2, MiscError=3 };

  int exec(const char *pass, const char *name);

  QByteArray error() { return m_Error; }

private:
  int ConverseChfn(const char *pass);

  QByteArray m_Error;
};

#endif
