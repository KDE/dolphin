/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#ifndef __Passwd_h_Included__
#define __Passwd_h_Included__

#include <qcstring.h>
#include <kdesu/process.h>

/**
 * A C++ API to passwd.
 */

class PasswdProcess
    : public PtyProcess
{
public:
    PasswdProcess(QCString user=0);
    ~PasswdProcess();

    enum Errors { PasswdNotFound=1, PasswordIncorrect, PasswordNotGood };

    int checkCurrent(const char *oldpass);
    int exec(const char *oldpass, const char *newpass, int check=0);

    QCString error() { return m_Error; }

private:
    bool isPrompt(QCString line, const char *word=0L);
    int ConversePasswd(const char *oldpass, const char *newpass,
	    int check);

    QCString m_User, m_Error;
    bool bOtherUser;
};


#endif // __Passwd_h_Included__
