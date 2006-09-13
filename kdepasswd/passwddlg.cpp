/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#include <klocale.h>
#include <kmessagebox.h>

#include "passwd.h"
#include "passwddlg.h"

KDEpasswd1Dialog::KDEpasswd1Dialog()
    : KPasswordDialog(Password, false, 0)
{
    setCaption(i18n("Change Password"));
    setPrompt(i18n("Please enter your current password:"));
}


KDEpasswd1Dialog::~KDEpasswd1Dialog()
{
}


bool KDEpasswd1Dialog::checkPassword(const char *password)
{
    PasswdProcess proc(0);

    int ret = proc.checkCurrent(password);
    switch (ret)
    {
    case -1:
    {
        QString msg = QString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Conversation with 'passwd' failed.") + msg;
	KMessageBox::error(this, msg);
	done(Rejected);
	return false;
    }

    case 0:
	return true;

    case PasswdProcess::PasswdNotFound:
	KMessageBox::error(this, i18n("Could not find the program 'passwd'."));
	done(Rejected);
	return false;

    case PasswdProcess::PasswordIncorrect:
        KMessageBox::sorry(this, i18n("Incorrect password. Please try again."));
	return false;

    default:
	KMessageBox::error(this, i18n("Internal error: illegal return value "
		"from PasswdProcess::checkCurrent."));
	done(Rejected);
	return false;
    }
}


// static
int KDEpasswd1Dialog::getPassword(QByteArray &password)
{
    KDEpasswd1Dialog *dlg = new KDEpasswd1Dialog();
    int res = dlg->exec();
    if (res == Accepted)
	password = dlg->password();
    delete dlg;
    return res;
}



KDEpasswd2Dialog::KDEpasswd2Dialog(const char *oldpass, QByteArray user)
    : KPasswordDialog(NewPassword, false, 0)
{
    m_Pass = oldpass;
    m_User = user;

    setCaption(i18n("Change Password"));
    if (m_User.isEmpty())
        setPrompt(i18n("Please enter your new password:"));
    else
        setPrompt(i18n("Please enter the new password for user <b>%1</b>:", QString::fromLocal8Bit(m_User)));
}


KDEpasswd2Dialog::~KDEpasswd2Dialog()
{
}


bool KDEpasswd2Dialog::checkPassword(const char *password)
{
    PasswdProcess proc(m_User);

    if (strlen(password) > 8)
    {
	switch(KMessageBox::warningYesNoCancel(this,
		m_User.isEmpty() ?
		i18n("Your password is longer than 8 characters. On some "
			"systems, this can cause problems. You can truncate "
			"the password to 8 characters, or leave it as it is.") :
		i18n("The password is longer than 8 characters. On some "
			"systems, this can cause problems. You can truncate "
			"the password to 8 characters, or leave it as it is.")
			,
		i18n("Password Too Long"),
		KGuiItem(i18n("Truncate")),
		KGuiItem(i18n("Use as Is")),
		"truncatePassword"))
	{
	case KMessageBox::Yes :
		const_cast<char *>(password)[8] = '\000';
		break;
	case KMessageBox::No :
		break;
	default : return false;
	}
    }

    int ret = proc.exec(m_Pass, password);
    switch (ret)
    {
    case 0:
    {
        hide();
        QString msg = QString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Your password has been changed.") + msg;
        KMessageBox::information(0L, msg);
        return true;
    }

    case PasswdProcess::PasswordNotGood:
    {
        QString msg = QString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Your password has not been changed.") + msg;

        // The pw change did not succeed. Print the error.
        KMessageBox::sorry(this, msg);
        return false;
    }

    default:
        QString msg = QString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Conversation with 'passwd' failed.") + msg;
	KMessageBox::sorry(this, msg);
	done(Rejected);
	return true;
    }

    return true;
}


#include "passwddlg.moc"
