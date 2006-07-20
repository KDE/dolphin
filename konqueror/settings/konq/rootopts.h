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


#include <QString>

#include <kcmodule.h>
#include <kio/global.h>
#include <kurl.h>

class QCheckBox;
class QComboBox;
class QPushButton;

class K3ListView;
class KJob;
class KUrlRequester;
class QStringList;

namespace KIO { class Job; }

//-----------------------------------------------------------------------------
// The "Path" Tab contains :
// The paths for Desktop, Autostart and Documents

class DesktopPathConfig : public KCModule
{
        Q_OBJECT
public:
        DesktopPathConfig( QWidget *parent, const QStringList &args );
        virtual void load();
        virtual void save();
        virtual void defaults();

private Q_SLOTS:
    void slotEntries( KIO::Job * job, const KIO::UDSEntryList& list);

private:
        // Desktop Paths
        KUrlRequester *urDesktop;
        KUrlRequester *urAutostart;
        KUrlRequester *urDocument;

        bool moveDir( const KUrl & src, const KUrl & dest, const QString & type );
        bool m_ok;
        KUrl m_copyToDest; // used when the destination directory already exists
        KUrl m_copyFromSrc;

private Q_SLOTS:
        void slotResult( KJob * job );
};

#endif // __ROOT_OPTIONS_H
