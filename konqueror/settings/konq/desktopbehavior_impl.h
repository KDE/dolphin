/**
 * (c) Martin R. Jones 1996
 * (c) David Faure 1998, 2000
 * (c) John Firebaugh 2003
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

#ifndef desktopbehavior_h
#define desktopbehavior_h

#include "desktopbehavior.h"
#include <kconfig.h>
#include <kcmodule.h>
class QStringList;

class DesktopBehavior : public DesktopBehaviorBase
{
        Q_OBJECT
public:
        DesktopBehavior(KSharedConfig::Ptr config, QWidget *parent = 0, const char *name = 0);
        virtual void load();
        virtual void save();
        virtual void defaults();
        virtual QString quickHelp() const;
        friend class DesktopBehaviorPreviewItem;
	friend class DesktopBehaviorMediaItem;

Q_SIGNALS:
        void changed();

private Q_SLOTS:
        void enableChanged();
	void comboBoxChanged();
	void editButtonPressed();

private:
        KSharedConfig::Ptr g_pConfig;

	void fillMediaListView();
	void saveMediaListView();

        // Combo for the menus
        void fillMenuCombo( QComboBox * combo );

        typedef enum { NOTHING = 0, WINDOWLISTMENU, DESKTOPMENU, APPMENU, BOOKMARKSMENU=12 } menuChoice;
        bool m_bHasMedia;
};

class DesktopBehaviorModule : public KCModule
{
        Q_OBJECT

public:
        DesktopBehaviorModule(QWidget *parent, const QStringList &);
        virtual void load() { m_behavior->load(); emit KCModule::changed( false ); }
        virtual void save() { m_behavior->save(); emit KCModule::changed( false ); }
        virtual void defaults() { m_behavior->defaults(); emit KCModule::changed( true ); }

private Q_SLOTS:
        void changed();

private:
        DesktopBehavior* m_behavior;
};

#endif
