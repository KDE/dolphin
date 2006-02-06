/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#include <config.h>

#include <kuniqueapplication.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kuser.h>
#include <kdebug.h>

#include "passwd.h"
#include "passwddlg.h"

static KCmdLineOptions options[] = 
{
    { "+[user]", I18N_NOOP("Change password of this user"), 0 },
    KCmdLineLastOption
};


int main(int argc, char **argv)
{
    KAboutData aboutData("kdepasswd", I18N_NOOP("KDE passwd"),
            VERSION, I18N_NOOP("Changes a UNIX password."),
            KAboutData::License_Artistic, "Copyright (c) 2000 Geert Jansen");
    aboutData.addAuthor("Geert Jansen", I18N_NOOP("Maintainer"),
            "jansen@kde.org", "http://www.stack.nl/~geertj/");
 
    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions(options);
    KUniqueApplication::addCmdLineOptions();


    if (!KUniqueApplication::start()) {
	kDebug() << "kdepasswd is already running" << endl;
	return 0;
    }

    KUniqueApplication app;

    KUser ku;
    QByteArray user;
    bool bRoot = ku.isSuperUser();
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (args->count())
	user = args->arg(0);

    /* You must be able to run "kdepasswd loginName" */
    if ( !user.isEmpty() && user!=KUser().loginName().toUtf8() && !bRoot)
    {
        KMessageBox::sorry(0, i18n("You need to be root to change the password of other users."));
        return 0;
    }

    QByteArray oldpass;
    if (!bRoot)
    {
        int result = KDEpasswd1Dialog::getPassword(oldpass);
        if (result != KDEpasswd1Dialog::Accepted)
	    return 0;
    }

    KDEpasswd2Dialog *dlg = new KDEpasswd2Dialog(oldpass, user);


    dlg->exec();

    return 0;
}

