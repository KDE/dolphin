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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>


#include <kdebug.h>

#include "passwd.h"


PasswdProcess::PasswdProcess(QCString user)
{
    struct passwd *pw;

    if (user.isEmpty()) 
    {
	pw = getpwuid(getuid());
	if (pw == 0L) 
	{
	    kdDebug(1512) << "You don't exist!\n";
            return;
	}
	m_User = pw->pw_name;
    } else 
    {
	pw = getpwnam(user);
	if (pw == 0L) 
	{
	    kdDebug(1512) << k_lineinfo << "User " << user << "does not exist.\n";
	    return;
	}
	m_User = user;
    }
    m_ThisUser = pw->pw_name;
}


PasswdProcess::~PasswdProcess()
{
}


int PasswdProcess::checkCurrent(const char *oldpass)
{
    return exec(oldpass, 0L, 1);
}
    

int PasswdProcess::exec(const char *oldpass, const char *newpass,
	int check)
{    
    if (m_User.isEmpty())
	return -1;
    if (check)
	setTerminal(true);

    // Try to set the default locale to make the parsing of the output 
    // of `passwd' easier.
    putenv("LANG=C");

    QCStringList args;
    if (m_User != m_ThisUser)
        args += m_User;
    int ret = PtyProcess::exec("passwd", args);
    if (ret < 0)
    {
	kdDebug(1512) << k_lineinfo << "Passwd not found!\n";
	return PasswdNotFound;
    }

    ret = ConversePasswd(oldpass, newpass, check);
    if (ret < 0)
	kdDebug(1512) << k_lineinfo << "Conversation with passwd failed.\n";

    waitForChild();
    return ret;
}


/*
 * The tricky thing is to make this work with a lot of different passwd
 * implementations. We _don't_ want implementation specific routines.
 * Return values: -1 = unknown error, 0 = ok, >0 = error code.
 */

int PasswdProcess::ConversePasswd(const char *oldpass, const char *newpass, 
	int check)
{
    QCString line;
    int state = 0;

    while (state != 7)
    {
	line = readLine();
	if (line.isNull()) 
	{
	    return -1;
	}

        if (state == 0 && isPrompt(line, "new"))
            // If root is changing a user's password,
            // passwd can't prompt for the original password.
            // Therefore, we have to start at state=2.
            state=2;

	switch (state) 
	{
	case 0:
	    // Eat garbage, wait for prompt
	    if (isPrompt(line, "password")) 
	    {
		WaitSlave();
		write(m_Fd, oldpass, strlen(oldpass));
		write(m_Fd, "\n", 1);
		state++; 
		break;
	    }
	    if (m_bTerminal) 
		fputs(line, stdout);
	    break;
	
	case 1: case 3: case 6:
	    // Wait for \n
	    if (line.isEmpty()) 
	    {
		state++;
		break;
	    }
	    // error
	    return -1;

	case 2: 
	    // Wait for second prompt.
	    if (isPrompt(line, "new")) 
	    {
		if (check)
		{
		    kill(m_Pid, SIGKILL);
		    waitForChild();
		    return 0;
		}
		WaitSlave();
		write(m_Fd, newpass, strlen(newpass));
		write(m_Fd, "\n", 1);
		state++; 
		break;
	    }
	    // Assume incorrect password.
	    if (m_bTerminal) 
		fputs(line, stdout);
	    m_Error = line;
	    return PasswordIncorrect;

	case 4:
	    // Wait for third prompt
	    if (isPrompt(line, "re")) 
	    {
		WaitSlave();
		write(m_Fd, newpass, strlen(newpass));
		write(m_Fd, "\n", 1);
		state += 2;
		break;
	    }
	    // Warning or error about the new password
	    if (m_bTerminal) 
		fputs(line, stdout);
	    m_Error = line;
	    state++;
	    break;

	case 5:
	    // Wait for either a "Reenter password" or a "Enter password" prompt
	    if (isPrompt(line, "re"))
	    {
		WaitSlave();
		write(m_Fd, newpass, strlen(newpass));
		write(m_Fd, "\n", 1);
		state++;
		break;
	    }
	    else if (isPrompt(line, "password"))
	    {
		kill(m_Pid, SIGKILL);
		waitForChild();
		return PasswordNotGood;
	    }
	    if (m_bTerminal)
		fputs(line, stdout);
	    m_Error += line;
	    break;
	}
    }

    kdDebug(1512) << k_lineinfo << "Conversation ended successfully.\n";
    return 0;
}
    

bool PasswdProcess::isPrompt(QCString line, const char *word)
{
    unsigned i, j, colon;

    for (i=0,j=0,colon=0; i<line.length(); i++) 
    {
	if (line[i] == ':') 
	{
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


