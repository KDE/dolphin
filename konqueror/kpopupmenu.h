#ifndef __kpopupmenu_h
#define __kpopupmenu_h

#include "konqueror.h"
#include <qobject.h>
#include <qmap.h>
#include <qstringlist.h>

#include <kmimetypes.h>

class KService;

/** TODO : docu ! */
class KonqPopupMenu : public QObject
{
  Q_OBJECT
public:
  KonqPopupMenu( const Konqueror::View::MenuPopupRequest &popup, 
                 QString viewURL,
                 bool canGoBack, 
                 bool canGoForward, 
                 bool canGoUp );
  ~KonqPopupMenu() {}

public slots:
  void slotFileNewActivated( CORBA::Long id );
  void slotFileNewAboutToShow();
  void slotPopupNewView();
  void slotPopupEmptyTrashBin();
  void slotPopupCopy();
  void slotPopupPaste();
  void slotPopupTrash();
  void slotPopupDelete();
  void slotPopupOpenWith();
  void slotPopupBookmarks();
  void slotPopup( int id );
  void slotPopupProperties();

protected:
  KNewMenu *m_pMenuNew;
  QString m_sViewURL;
  QStringList m_lstPopupURLs;
  CORBA::Long m_popupMode; // mode_t of first URL in m_lstPopupURLs
  QMap<int,const KService *> m_mapPopup;
  QMap<int,KDELnkMimeType::Service> m_mapPopup2;
};

#endif
