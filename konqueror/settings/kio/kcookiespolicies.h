/**
 * kcookiespolicies.h - Cookies configuration
 *
 * Original Authors
 * Copyright (c) Waldo Bastian <bastian@kde.org>
 * Copyright (c) 1999 David Faure <faure@kde.org>
 *
 * Re-written by:
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __KCOOKIESPOLICIES_H
#define __KCOOKIESPOLICIES_H

#include <qmap.h>
#include <kcmodule.h>

class QGroupBox;
class QCheckBox;
class QPushButton;
class QRadioButton;
class QButtonGroup;
class QStringList;
class QListViewItem;

class KListView;
class DCOPClient;

class KCookiesPolicies : public KCModule
{
    Q_OBJECT

public:
    KCookiesPolicies(QWidget *parent = 0L, const char *name = 0L);
    ~KCookiesPolicies();

    virtual void load();
    virtual void save();
    virtual void defaults();
    virtual QString quickHelp() const;

protected slots:
    void changeCookiesEnabled();
    void updateButtons();

    void deleteAllPressed();
    void deletePressed();
    void changePressed();
    void addPressed();
    void changed();

private:
    void updateDomainList(const QStringList& list);
    bool handleDuplicate( const QString& domain, int );

    // Global Policy Cookies enabled
    QGroupBox*    gb_global;
    QButtonGroup* bg_default;
    QCheckBox*    cb_enableCookies;
    QRadioButton* rb_gbPolicyAccept;
    QRadioButton* rb_gbPolicyAsk;
    QRadioButton* rb_gbPolicyReject;

    // Domain specific cookie policies
    QGroupBox*    gb_domainSpecific;
    KListView*    lv_domainPolicy;
    QPushButton*  pb_domPolicyAdd;
    QPushButton*  pb_domPolicyDelete;
    QPushButton*  pb_domPolicyDeleteAll;
    QPushButton*  pb_domPolicyChange;

    QMap<QListViewItem*, const char *> domainPolicy;
};

#endif // __KCOOKIESPOLICIES_H
