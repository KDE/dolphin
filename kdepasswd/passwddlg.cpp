/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#include <qstring.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include "passwd.h"
#include "passwddlg.h"


KDEpasswd1Dialog::KDEpasswd1Dialog(QCString user)
    : KPasswordDialog(Password, "")
{
    m_User = user;

    setCaption(i18n("Change password"));
    setPrompt(i18n("Please enter your current password."));
}


KDEpasswd1Dialog::~KDEpasswd1Dialog()
{
}


bool KDEpasswd1Dialog::checkPassword(const char *password)
{
    PasswdProcess proc(m_User);
    int ret = proc.checkCurrent(password);
    if (ret != 0) {
        KMessageBox::sorry(this, i18n("Incorrect password! Please try again."));
	return false;
    }
    return true;
}


// static
int KDEpasswd1Dialog::getPassword(QCString &password, QCString user)
{
    KDEpasswd1Dialog *dlg = new KDEpasswd1Dialog(user);
    int res = dlg->exec();
    if (res == Accepted)
	password = dlg->password();
    delete dlg;
    return res;
}



KDEpasswd2Dialog::KDEpasswd2Dialog(const char *oldpass, QCString user)
    : KPasswordDialog(NewPassword, "")
{
    m_Pass = oldpass;
    m_User = user;

    setCaption(i18n("Change password"));
    setPrompt(i18n("Please enter your new password."));
}


KDEpasswd2Dialog::~KDEpasswd2Dialog()
{
}


bool KDEpasswd2Dialog::checkPassword(const char *password)
{
    PasswdProcess proc(m_User);
    int ret = proc.exec(m_Pass, password);
    if (ret != 0) {
        KMessageBox::error(this, proc.error());
	return false;
    }
    return true;
}


#include "passwddlg.moc"
