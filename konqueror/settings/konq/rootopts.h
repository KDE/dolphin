//
//
// "Desktop Icons Options" Tab for KDesktop configuration
//
// (c) Martin R. Jones 1996
//
// Port to KControl, split from "Misc" Tab, Port to KControl2
// (c) David Faure 1998
// Desktop menus, paths
// (c) David Faure 2000

#ifndef __ROOT_OPTIONS_H
#define __ROOT_OPTIONS_H


#include <qstring.h>

#include <kcmodule.h>
#include <kio/global.h>
#include <kurl.h>

class QCheckBox;
class QComboBox;
class QPushButton;

class KConfig;
class KListView;
class KURLRequester;

namespace KIO { class Job; }

//-----------------------------------------------------------------------------
// The "Path" Tab contains :
// The paths for Desktop, Autostart and Documents

class DesktopPathConfig : public KCModule
{
        Q_OBJECT
public:
        DesktopPathConfig(QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();

private slots:
    void slotEntries( KIO::Job * job, const KIO::UDSEntryList& list);

private:
        // Desktop Paths
        KURLRequester *urDesktop;
        KURLRequester *urAutostart;
        KURLRequester *urDocument;

        bool moveDir( const KURL & src, const KURL & dest, const QString & type );
        bool m_ok;
        KURL m_copyToDest; // used when the destination directory already exists
        KURL m_copyFromSrc;

private slots:
        void slotResult( KIO::Job * job );
};

#endif // __ROOT_OPTIONS_H
