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

#include "policydlg.h"

class QGroupBox;
class QCheckBox;
class QPushButton;
class QRadioButton;
class QButtonGroup;
class QStringList;
class QListViewItem;

class KListView;
class DCOPClient;

class KCookiesPolicies : public QWidget
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
    void autoAcceptSessionCookies ( bool );
    void ignoreCookieExpirationDate ( bool );
    void cookiesEnabled( bool );
    void emitChanged();    
        
    void selectionChanged();
    void updateButtons();

    void deleteAllPressed();
    void deletePressed();
    void changePressed();
    void addPressed();

signals:
    void changed( bool );

private:
    void updateDomainList(const QStringList& list);
    bool handleDuplicate( const QString& domain, int );
    void splitDomainAdvice (const QString& configStr, QString &domain,
                            KCookieAdvice::Value &advice);

    int d_itemsSelected;

    // Global Policy Cookies enabled
    QGroupBox*    m_gbGlobal;
    QButtonGroup* m_bgDefault;
    
    QCheckBox*    m_cbEnableCookies;
    QCheckBox*    m_cbRejectCrossDomainCookies;
    QCheckBox*    m_cbAutoAcceptSessionCookies;
    QCheckBox*    m_cbIgnoreCookieExpirationDate;
    
    QRadioButton* m_rbPolicyAccept;
    QRadioButton* m_rbPolicyAsk;
    QRadioButton* m_rbPolicyReject;

    // Domain specific cookie policies
    QGroupBox*    m_gbDomainSpecific;
    KListView*    m_lvDomainPolicy;
    QPushButton*  m_pbAdd;
    QPushButton*  m_pbDelete;
    QPushButton*  m_pbDeleteAll;
    QPushButton*  m_pbChange;
    
    // Generic settings group;
    QButtonGroup* m_bgPreferences;

    QMap<QListViewItem*, const char *> m_pDomainPolicy;
};

#endif // __KCOOKIESPOLICIES_H
