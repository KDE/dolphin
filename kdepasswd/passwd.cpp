/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 1999,2000 Geert Jansen <jansen@kde.org>
 * 
 * passwd.cpp: Change a user's password.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <qcstring.h>

#include <kdebug.h>
#include <kstddirs.h>

#include <kdesu/process.h>
#include "passwd.h"


#ifdef __GNUC__
#define ID __PRETTY_FUNCTION__
#else
#define ID "PasswdProcess"
#endif


PasswdProcess::PasswdProcess(QCString user)
{
    struct passwd *pw;

    if (user.isEmpty()) {
	pw = getpwuid(getuid());
	if (pw == 0L) {
	    kdDebug() << "You don't exist!";
            return;
	}
	m_User = pw->pw_name;
    } else {
	pw = getpwnam(user);
	if (pw == 0L) {
	    kdDebug() << ID << ": User " << user.data() << "does not exist.";
	    return;
	}
	m_User = user;
    }
}


PasswdProcess::~PasswdProcess()
{
}


int PasswdProcess::checkCurrent(const char *oldpass)
{
    return exec(oldpass, 0L, 1) != 1;
}
    

int PasswdProcess::exec(const char *oldpass, const char *newpass,
	int check)
{    
    if (m_User.isEmpty())
	return -1;
    if (check)
	setTerminal(true);

    QString path = KStandardDirs::findExe("passwd");
    if (path.isEmpty()) {
	kdDebug() << "passwd not found!";
	return -1;
    }
    QCString cpath = path.latin1();
    QCStringList args;
    args += m_User;
    if (PtyProcess::exec(cpath, args) < 0)
	return -1;

    int ret = ConversePasswd(oldpass, newpass, check);
    if (ret < 0) {
	kdDebug() << ID << ": Conversation with passwd failed";
	return -1;
    } 
    if (ret == 1) {
	kill(m_Pid, SIGKILL);
	waitForChild();
    }
    return ret;
}


/*
 * The tricky thing is to make this work with a lot of different passwd
 * implementations. We _don't_ want implementation specific routines.
 */

int PasswdProcess::ConversePasswd(const char *oldpass, const char *newpass, 
	int check)
{
    QCString line;
    int state = 0;

    while (state < 7) {
	line = readLine();
	if (line.isNull()) {
	    if (state == 6)
		break;
	    else
		return -1;
	}

	switch (state) {
	case 0:
	    // Eat garbage, wait for prompt
	    if (isPrompt(line, "password")) {
		WaitSlave();
		write(m_Fd, oldpass, strlen(oldpass));
		write(m_Fd, "\n", 1);
		state++; break;
	    }
	    if (m_bTerminal) fputs(line, stdout);
	    break;
	
	case 1: case 3: case 5:
	    // Wait for \n
	    if (line.isEmpty()) {
		state++;
		break;
	    }
	    // error
	    return -1;

	case 2: 
	    // Wait for second prompt.
	    if (isPrompt(line, "new")) {
		if (check)
		    return 1;
		WaitSlave();
		write(m_Fd, newpass, strlen(newpass));
		write(m_Fd, "\n", 1);
		state++; break;
	    }
	    // Assume error message.
	    if (m_bTerminal) fputs(line, stdout);
	    m_Error = line;
	    return -1;

	case 4:
	    // Wait for third prompt
	    if (isPrompt(line, "password")) {
		WaitSlave();
		write(m_Fd, newpass, strlen(newpass));
		write(m_Fd, "\n", 1);
		state++; break;
	    }
	    // Assume error message.
	    if (m_bTerminal) fputs(line, stdout);
	    m_Error = line;
	    return -1;

	case 6:
	    // Wait for EOF, but no prompt.
	    if (isPrompt(line, "password"))
		return 1;
	    if (m_bTerminal) fputs(line, stdout);
	}
    }

    kdDebug() << ID << ": Conversation ended";
    return 0;
}
    

bool PasswdProcess::isPrompt(QCString line, const char *word)
{
    unsigned i, j, colon;

    for (i=0,j=0,colon=0; i<line.length(); i++) {
	if (line[i] == ':') {
	    j = i; colon++;
	    continue;
	}
	if (!isspace(line[i]))
	    j++;
    }

    if ((colon != 1) || (line[j] != ':'))
	return false;
    if (word == 0L)
	return true;

    return line.contains(word, false);
}


