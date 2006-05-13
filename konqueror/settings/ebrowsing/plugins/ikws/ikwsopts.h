/*
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
 * Copyright (c) 2002, 2003 Dawit Alemayehu <adawit@kde.org>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __IKWSOPTS_H___
#define __IKWSOPTS_H___

#include <QLayout>
#include <QTabWidget>

#include <kcmodule.h>
#include <kservice.h>

class FilterOptionsUI;
class SearchProvider;
class SearchProviderItem;

class FilterOptions : public KCModule
{
    Q_OBJECT

public:
    FilterOptions(KInstance *instance, QWidget *parent = 0);

    void load();
    void save();
    void defaults();
    QString quickHelp() const;

protected Q_SLOTS:
    void configChanged();
    void checkFavoritesChanged();

    void setWebShortcutState();

    void addSearchProvider();
    void changeSearchProvider();
    void deleteSearchProvider();
    void updateSearchProvider();

private:
    SearchProviderItem *displaySearchProvider(SearchProvider *p, bool fallback = false);

    void setDelimiter (char);
    char delimiter ();

    // The names of the providers that the user deleted,
    // these are marked as deleted in the user's homedirectory
    // on save if a global service file exists for it.
    QStringList m_deletedProviders;
    QMap <QString, QString> m_defaultEngineMap;
    QStringList m_favoriteEngines;

    FilterOptionsUI* m_dlg;
};

#endif
